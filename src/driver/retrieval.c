#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "pfordelta/opt_p4.h"
#include "dictionary/Dictionary.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "util/ParseCommandLine.h"
#include "PostingsPool.h"
#include "Pointers.h"
#include "Config.h"
#include "InvertedIndex.h"
#include "intersection/SvS.h"
#include "intersection/WAND.h"
#include "intersection/WANDPositions.h"
#include "heap/HeapWithPositions.h"
#include "heap/Heap.h"
#include "intersection/BWAND_AND.h"
#include "intersection/BWAND_OR.h"
#include "scorer/ScoringFunction.h"
#include "scorer/BM25.h"
#include "scorer/Dirichlet.h"
#include "feature/TermFeature.h"
#include "feature/OrderedWindowSequentialDependenceFeature.h"
#include "feature/UnorderedWindowSequentialDependenceFeature.h"
#include "model/trees/TreeBuilder.h"

#ifndef RETRIEVAL_ALGO_ENUM_GUARD
#define RETRIEVAL_ALGO_ENUM_GUARD
typedef enum Algorithm Algorithm;
enum Algorithm {
  SVS = 0,
  WAND = 1,
  MBWAND = 2,
  BWAND_OR = 3,
  BWAND_AND = 4,
  WAND_FEATURES = 5,
  MBWAND_FEATURES = 6,
};
#endif

typedef float (*computeFeature)(int** positions, int* query, int qlength, int docid,
                                Pointers* pointers, ScoringFunction* scorer);

int main (int argc, char** args) {
  // Index path
  char* inputPath = getValueCL(argc, args, "-index");
  // Query path
  char* queryPath = getValueCL(argc, args, "-query");
  // Output path (optional)
  char* outputPath = getValueCL(argc, args, "-output");
  // Hits
  int hits = 1000;
  int hitsSpecified = 0;
  if(isPresentCL(argc, args, "-hits")) {
    hitsSpecified = 1;
    hits = atoi(getValueCL(argc, args, "-hits"));
  }
  // Algorithm
  char* intersectionAlgorithm = getValueCL(argc, args, "-algorithm");
  Algorithm algorithm = SVS;

  // Algorithm is limited to the following list (case sensitive):
  // - SvS (conjunctive)
  // - WAND (disjunctive)
  if(!strcmp(intersectionAlgorithm, "SvS")) {
    algorithm = SVS;
  } else if(!strcmp(intersectionAlgorithm, "WAND")) {
    algorithm = WAND;
  } else if(!strcmp(intersectionAlgorithm, "WANDFeatures")) {
    algorithm = WAND_FEATURES;
  } else if(!strcmp(intersectionAlgorithm, "MBWANDFeatures")) {
    algorithm = MBWAND_FEATURES;
  } else if(!strcmp(intersectionAlgorithm, "MBWAND")) {
    algorithm = MBWAND;
  } else if(!strcmp(intersectionAlgorithm, "BWAND_OR")) {
    algorithm = BWAND_OR;
  } else if(!strcmp(intersectionAlgorithm, "BWAND_AND")) {
    algorithm = BWAND_AND;
  } else {
    printf("Invalid algorithm (Options: SvS | WAND | ");
    printf("MBWAND | BWAND_OR | BWAND_AND | WANDFeatures | MBWANDFeatures)\n");
    return;
  }

  // Read the inverted index
  InvertedIndex* index = readInvertedIndex(inputPath);

  // Feature extraction
  computeFeature* extractors = NULL;
  ScoringFunction* scorers = NULL;
  int numberOfFeatures = 0;
  if((index->vectors || algorithm == WAND_FEATURES || algorithm == MBWAND_FEATURES) &&
     isPresentCL(argc, args, "-features")) {
    char* featurePath = getValueCL(argc, args, "-features");
    FILE* fp = fopen(featurePath, "r");
    int f;
    char featureInputText[1024];

    fscanf(fp, "%d", &numberOfFeatures);
    extractors = calloc(numberOfFeatures, sizeof(computeFeature));
    scorers = calloc(numberOfFeatures, sizeof(ScoringFunction));
    for(f = 0; f < numberOfFeatures; f++) {
      fscanf(fp, "%s", featureInputText);
      if(!strcmp(featureInputText, "BM25")) {
        scorers[f].function = BM25;
        BM25Parameter* param = calloc(1, sizeof(BM25Parameter));
        int i;
        float value = 0;
        for(i = 0; i < 2; i++) {
          fscanf(fp, "%[ ]%[^:]:%f", featureInputText, featureInputText, &value);
          if(!strcmp(featureInputText, "K1")) {
            param->K1 = value;
          } else if(!strcmp(featureInputText, "B")) {
            param->B = value;
          }
        }
        scorers[f].parameters = (void*) param;
      } else if(!strcmp(featureInputText, "Dirichlet")) {
        scorers[f].function = DIRICHLET;
        DirichletParameter* param = calloc(1, sizeof(DirichletParameter));
        fscanf(fp, "%[ ]%[^:]:%f", featureInputText, featureInputText, &param->MU);
        scorers[f].parameters = (void*) param;
      }

      fscanf(fp, "%s", featureInputText);
      if(!strcmp(featureInputText, "Term")) {
        scorers[f].phrase = 0;
        extractors[f] = computeTermFeature;
      } else if(!strcmp(featureInputText, "OD")) {
        extractors[f] = computeOrderedWindowSDFeature;
        fscanf(fp, "%[ ]%[^:]:%d", featureInputText, featureInputText, &scorers[f].phrase);
      } else if(!strcmp(featureInputText, "UW")) {
        extractors[f] = computeUnorderedWindowSDFeature;
        int window;
        fscanf(fp, "%[ ]%[^:]:%d", featureInputText, featureInputText, &window);
        scorers[f].phrase = window * 2;
      }
    }
  }

  // Read LambdaMART model
  TreeModel* treeModel = NULL;
  float* scores = NULL;
  Heap* rankedList = initHeap(hits);
  if(isPresentCL(argc, args, "-model")) {
    treeModel = parseTrees(getValueCL(argc, args, "-model"));

    int nb = hits;
    if(nb % V != 0) {
      nb = ((nb/V) + 1) * V;
    }
    scores = malloc(nb * sizeof(float));
  }

  // Read queries. Query file must be in the following format:
  // - First line: <number of queries: integer>
  // - <query id: integer> <query length: integer> <query text: string>
  // Note that, if a query term does not have a corresponding postings list,
  // then we drop the query term from the query. Empty queries are not evaluated.
  FixedIntCounter* queryLength = createFixedIntCounter(32768, 0);
  FixedIntCounter* idToIndexMap = createFixedIntCounter(32768, 0);
  FILE* fp = fopen(queryPath, "r");
  int totalQueries = 0, id, qlen, fqlen, j, pos, termid, i;
  char query[1024];
  fscanf(fp, "%d", &totalQueries);
  unsigned int** queries = (unsigned int**) malloc(totalQueries * sizeof(unsigned int*));
  for(i = 0; i < totalQueries; i++) {
    fscanf(fp, "%d %d", &id, &qlen);
    queries[i] = (unsigned int*) malloc(qlen * sizeof(unsigned int));
    pos = 0;
    fqlen = qlen;
    for(j = 0; j < qlen; j++) {
      fscanf(fp, "%s", query);
      termid = getTermId(index->dictionary, query);
      if(termid >= 0) {
        if(getHeadPointer(index->pointers, termid) != UNDEFINED_POINTER) {
          queries[i][pos++] = termid;
        } else {
          fqlen--;
        }
      } else {
        fqlen--;
      }
    }
    setFixedIntCounter(idToIndexMap, id, i);
    setFixedIntCounter(queryLength, id, fqlen);
  }
  fclose(fp);

  if(outputPath) {
    fp = fopen(outputPath, "w");
  }

  // Evaluate queries by iterating over the queries that are not empty
  id = -1;
  while((id = nextIndexFixedIntCounter(queryLength, id)) != -1) {
    // Measure elapsed time
    struct timeval start, end;
    gettimeofday(&start, NULL);

    qlen = queryLength->counter[id];
    int qindex = idToIndexMap->counter[id];

    unsigned int* qdf = (unsigned int*) calloc(qlen, sizeof(unsigned int));
    int* sortedDfIndex = (int*) calloc(qlen, sizeof(int));
    long* qHeadPointers = (long*) calloc(qlen, sizeof(long));

    qdf[0] = getDf(index->pointers, queries[qindex][0]);
    unsigned int minimumDf = qdf[0];
    for(i = 1; i < qlen; i++) {
      qdf[i] = getDf(index->pointers, queries[qindex][i]);
      if(qdf[i] < minimumDf) {
        minimumDf = qdf[i];
      }
    }

    // Sort query terms w.r.t. df
    if(algorithm == SVS || algorithm == BWAND_AND ||
       algorithm == BWAND_OR) {
      for(i = 0; i < qlen; i++) {
        unsigned int minDf = 0xFFFFFFFF;
        for(j = 0; j < qlen; j++) {
          if(qdf[j] < minDf) {
            minDf = qdf[j];
            sortedDfIndex[i] = j;
          }
        }
        qdf[sortedDfIndex[i]] = 0xFFFFFFFF;
      }
    } else {
      for(i = 0; i < qlen; i++) {
        sortedDfIndex[i] = i;
      }
    }

    for(i = 0; i < qlen; i++) {
      qHeadPointers[i] = getHeadPointer(index->pointers,
                                        queries[qindex][sortedDfIndex[i]]);
      qdf[i] = getDf(index->pointers, queries[qindex][sortedDfIndex[i]]);
    }

    // Compute intersection set (or in disjunctive mode, top-k)
    int* set = NULL;
    if(algorithm == SVS) {
      if(!hitsSpecified) {
        hits = minimumDf;
      }
      set = intersectSvS(index->pool, qHeadPointers, qlen, minimumDf, hits);
    } else if(algorithm == WAND || algorithm == MBWAND) {
      float* UB = (float*) malloc(qlen * sizeof(float));
      for(i = 0; i < qlen; i++) {
        int tf = getMaxTf(index->pointers, queries[qindex][sortedDfIndex[i]]);
        int dl = getMaxTfDocLen(index->pointers, queries[qindex][sortedDfIndex[i]]);
        if(algorithm == WAND) {
          UB[i] = _default_bm25(tf, qdf[i],
                                index->pointers->totalDocs, dl,
                                index->pointers->totalDocLen /
                                ((float) index->pointers->totalDocs));
        } else {
          UB[i] = idf(index->pointers->totalDocs, qdf[i]);
        }
      }
      set = wand(index->pool, qHeadPointers, qdf, UB, qlen,
                 index->pointers->docLen->counter,
                 index->pointers->totalDocs,
                 index->pointers->totalDocLen / (float) index->pointers->totalDocs,
                 hits, algorithm == MBWAND);
      free(UB);
    } else if(algorithm == BWAND_OR) {
      float* UB = (float*) malloc(qlen * sizeof(float));
      for(i = 0; i < qlen; i++) {
        UB[i] = idf(index->pointers->totalDocs, qdf[i]);
      }
      set = bwandOr(index->pool, qHeadPointers, UB, qlen, hits);
      free(UB);
    } else if(algorithm == BWAND_AND) {
      if(!hitsSpecified) {
        hits = minimumDf;
      }
      set = bwandAnd(index->pool, qHeadPointers, qlen, hits);
    }

    // Extract features
    float* features = NULL;
    int numberOfInstances = 0;
    if(numberOfFeatures > 0 && algorithm != WAND_FEATURES &&
       algorithm != MBWAND_FEATURES) {
      int f;
      features = malloc(hits * numberOfFeatures * sizeof(float));
      FixedBuffer** buffer = malloc(qlen * sizeof(FixedBuffer*));
      int** positions = malloc(qlen * sizeof(int*));
      for(f = 0; f < qlen; f++) {
        buffer[f] = createFixedBuffer(10);
      }

      for(i = 0; i < hits && set[i] > 0; i++) {
        getPositionsAsBuffers(index->vectors, set[i],
                              index->pointers->docLen->counter[set[i]],
                              queries[qindex], qlen, buffer);
        for(f = 0; f < qlen; f++) {
          positions[f] = buffer[f]->buffer;
        }
        for(f = 0; f < numberOfFeatures; f++) {
          features[i * numberOfFeatures + f] =
            extractors[f](positions, queries[qindex],
                          qlen, set[i], index->pointers, &scorers[f]);
        }
        numberOfInstances++;
      }
      for(f = 0; f < qlen; f++) {
        destroyFixedBuffer(buffer[f]);
      }
      free(buffer);
      free(positions);
    } else if(numberOfFeatures > 0 &&
              (algorithm == WAND_FEATURES || algorithm == MBWAND_FEATURES)) {
      float* UB = (float*) malloc(qlen * sizeof(float));
      for(i = 0; i < qlen; i++) {
        int tf = getMaxTf(index->pointers, queries[qindex][sortedDfIndex[i]]);
        int dl = getMaxTfDocLen(index->pointers, queries[qindex][sortedDfIndex[i]]);
        if(algorithm == WAND_FEATURES) {
          UB[i] = _default_bm25(tf, qdf[i],
                                index->pointers->totalDocs, dl,
                                index->pointers->totalDocLen /
                                ((float) index->pointers->totalDocs));
        } else {
          UB[i] = idf(index->pointers->totalDocs, qdf[i]);
        }
      }
      Candidate** candidates = wandPositions(index->pool, qHeadPointers, qdf, UB, qlen,
                                             index->pointers->docLen->counter,
                                             index->pointers->totalDocs,
                                             index->pointers->totalDocLen / (float) index->pointers->totalDocs,
                                             hits, algorithm == MBWAND_FEATURES);
      free(UB);

      set = calloc(hits, sizeof(int));
      features = malloc(hits * numberOfFeatures * sizeof(float));
      for(i = 0; candidates[i] != NULL; i++) {
        set[i] = candidates[i]->docid;
        int f;
        for(f = 0; f < numberOfFeatures; f++) {
          features[i * numberOfFeatures + f] =
            extractors[f](candidates[i]->positions, queries[qindex],
                          qlen, set[i], index->pointers, &scorers[f]);
        }
        numberOfInstances++;
        destroyCandidate(candidates[i], qlen);
      }
      free(candidates);
      if(i < hits) {
        set[i] = TERMINAL_DOCID;
      }
    }

    if(treeModel) {
      if(numberOfInstances % V != 0) {
        numberOfInstances = ((numberOfInstances/V) + 1) * V;
      }
      int leaf[V];
      int iIndex, tIndex, j;
      for(iIndex = 0; iIndex < numberOfInstances; iIndex+=V) {
        for(j = 0; j < V; j++) {
          scores[iIndex + j] = 0;
        }
        for(tIndex = 0; tIndex < treeModel->nbTrees; tIndex++) {
          findLeaf[treeModel->treeDepths[tIndex]](leaf, &features[iIndex * numberOfFeatures],
                                                  numberOfFeatures,
                                                  &treeModel->nodes[treeModel->nodeSizes[tIndex]]);
          for(j = 0; j < V; j++) {
            scores[iIndex + j] += treeModel->nodes[treeModel->nodeSizes[tIndex]+leaf[j]].theta;
          }
        }
      }

      clearHeap(rankedList);
      for(i = 0; i < hits && set[i] > 0; i++) {
        insertHeap(rankedList, set[i], scores[i]);
      }
      while(i > 0) {
        set[i - 1] = rankedList->docid[1];
        scores[i - 1] = rankedList->score[1];
        deleteMinHeap(rankedList);
        i--;
      }
    }

    // If output is specified, write the retrieved set to output
    if(outputPath) {
      for(i = 0; i < hits && set[i] > 0; i++) {
        if(!features && !treeModel) {
          fprintf(fp, "%d %d ", id, set[i]);
        } else if(features && !treeModel) {
          fprintf(fp, "%d %d ", id, set[i]);
          int f;
          for(f = 0; f < numberOfFeatures; f++) {
            fprintf(fp, "%d:%f ", (f + 1), features[i * numberOfFeatures + f]);
          }
        } else if(treeModel) {
          fprintf(fp, "%d Q0 %d %d %f zambezi", id, set[i], i + 1, scores[i]);
        }
        fprintf(fp, "\n");
      }
    }

    // Free the allocated memory
    if(features) free(features);
    free(set);
    free(qdf);
    free(sortedDfIndex);
    free(qHeadPointers);

    gettimeofday(&end, NULL);
    printf("%10.0f length: %d\n",
           ((float) ((end.tv_sec * 1000000 + end.tv_usec) -
                     (start.tv_sec * 1000000 + start.tv_usec))), qlen);
    fflush(stdout);
  }

  if(outputPath) {
    fclose(fp);
  }
  if(extractors) {
    for(i = 0; i < numberOfFeatures; i++) {
      free(scorers[i].parameters);
    }
    free(extractors);
    free(scorers);
  }
  for(i = 0; i < totalQueries; i++) {
    if(queries[i]) {
      free(queries[i]);
    }
  }
  if(treeModel) destroyTreeModel(treeModel);
  if(scores) free(scores);
  free(queries);
  destroyHeap(rankedList);
  destroyFixedIntCounter(queryLength);
  destroyFixedIntCounter(idToIndexMap);
  destroyInvertedIndex(index);
  return 0;
}
