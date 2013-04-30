/**
 * An inverted index data structure consisting of
 * the following components:
 *
 *  - SegmentPool, which contains all segments.
 *  - Dictionary, which is a mapping from term to term id
 *  - Pointers, which contains Document Frequency, Head/Tail Pointers,
 *    etc.
 *  - DocumentVectors, which contains compressed document vector representation
 *    of documents
 *
 * @author Nima Asadi
 */

#ifndef InvertedIndex_H_GUARD
#define InvertedIndex_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "dictionary/Dictionary.h"
#include "SegmentPool.h"
#include "Pointers.h"
#include "DocumentVector.h"
#include "Config.h"

typedef struct InvertedIndex InvertedIndex;

struct InvertedIndex {
  SegmentPool* pool;
  Dictionary** dictionary;
  Pointers* pointers;
  DocumentVector* vectors;
};

InvertedIndex* createInvertedIndex(int reverse, int indexVectors,
                                   int bloomEnabled, unsigned int nbHash,
                                   unsigned int bitsPerElement) {
  InvertedIndex* index = (InvertedIndex*) malloc(sizeof(InvertedIndex));
  index->pool = createSegmentPool(NUMBER_OF_POOLS, reverse, bloomEnabled,
                                   nbHash, bitsPerElement);
  index->dictionary = initDictionary();
  index->pointers = createPointers(DEFAULT_VOCAB_SIZE);
  index->vectors = NULL;
  if(indexVectors) {
    index->vectors = createDocumentVector(DEFAULT_COLLECTION_SIZE);
  }
  return index;
}

int indexDocumentVectors(InvertedIndex* index) {
  return index->vectors != NULL;
}

int hasValidPostingsList(InvertedIndex* index, int termid) {
  return getHeadPointer(index->pointers, termid) != UNDEFINED_POINTER;
}

/**
 * An iterator over terms with a valid Head Pointer.
 * Call this function as follows:
 *
 *   int term = -1;
 *   while((term = nextTerm(pointers, term)) != -1) {
 *     ...
 *   }
 */
int nextTermId(InvertedIndex* index, int currentTermId) {
  return nextIndexFixedLongCounter(index->pointers->headPointers, currentTermId);
}

int getDf_InvertedIndex(InvertedIndex* index, int term) {
  return getFixedIntCounter(index->pointers->df, term);
}

void destroyInvertedIndex(InvertedIndex* index) {
  destroySegmentPool(index->pool);
  destroyDictionary(index->dictionary);
  destroyPointers(index->pointers);
  if(index->vectors) {
    destroyDocumentVector(index->vectors);
  }
}

InvertedIndex* readInvertedIndex(char* rootPath) {
  InvertedIndex* index = (InvertedIndex*) malloc(sizeof(InvertedIndex));

  char dicPath[1024];
  strcpy(dicPath, rootPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);
  FILE* fp = fopen(dicPath, "rb");
  index->dictionary = readDictionary(fp);
  fclose(fp);

  char indexPath[1024];
  strcpy(indexPath, rootPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);
  fp = fopen(indexPath, "rb");
  index->pool = readSegmentPool(fp);
  fclose(fp);

  char pointerPath[1024];
  strcpy(pointerPath, rootPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);
  fp = fopen(pointerPath, "rb");
  index->pointers = readPointers(fp);
  fclose(fp);

  char vectorsPath[1024];
  strcpy(vectorsPath, rootPath);
  strcat(vectorsPath, "/");
  strcat(vectorsPath, DOCUMENT_VECTOR_FILE);
  if(!access(vectorsPath, F_OK)) {
    fp = fopen(vectorsPath, "rb");
    index->vectors = readDocumentVector(fp);
    fclose(fp);
  } else {
    index->vectors = NULL;
  }

  return index;
}

void writeInvertedIndex(InvertedIndex* index, char* rootPath) {
  char dicPath[1024];
  strcpy(dicPath, rootPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);
  FILE* ofp = fopen(dicPath, "wb");
  writeDictionary(index->dictionary, ofp);
  fclose(ofp);

  char indexPath[1024];
  strcpy(indexPath, rootPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);
  ofp = fopen(indexPath, "wb");
  writeSegmentPool(index->pool, ofp);
  fclose(ofp);

  char pointerPath[1024];
  strcpy(pointerPath, rootPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);
  ofp = fopen(pointerPath, "wb");
  writePointers(index->pointers, ofp);
  fclose(ofp);

  if(index->vectors) {
    char vectorsPath[1024];
    strcpy(vectorsPath, rootPath);
    strcat(vectorsPath, "/");
    strcat(vectorsPath, DOCUMENT_VECTOR_FILE);
    ofp = fopen(vectorsPath, "wb");
    writeDocumentVector(index->vectors, ofp);
    fclose(ofp);
  }
}

#endif
