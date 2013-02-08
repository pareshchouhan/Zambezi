#ifndef SCORER_H_GUARD
#define SCORER_H_GUARD

#include "Pointers.h"
#include "scorer/BM25.h"
#include "scorer/Dirichlet.h"

typedef enum FunctionName FunctionName;
enum FunctionName {
  BM25 = 0,
  DIRICHLET = 1,
};

typedef struct ScoringFunction ScoringFunction;
struct ScoringFunction {
  FunctionName function;
  int phrase;
  void* parameters;
};

float computeTermScoringFunction(ScoringFunction* scorer, int query,
                                 int docid, int tf, Pointers* pointers) {
  if(scorer->function == BM25) {
    return bm25(tf, pointers->df->counter[query], pointers->totalDocs,
                pointers->docLen->counter[docid],
                pointers->totalDocLen / (float) pointers->totalDocs,
                (BM25Parameter*) scorer->parameters);
  } else if(scorer->function == DIRICHLET) {
    return dirichlet(tf, pointers->docLen->counter[docid],
                     pointers->cf->counter[query], pointers->totalDocLen,
                     (DirichletParameter*) scorer->parameters);
  }
}

float computePhraseScoringFunction(ScoringFunction* scorer,
                                   int docid, int tf, Pointers* pointers) {
  if(scorer->function == BM25) {
    return bm25Phrase(tf, pointers->docLen->counter[docid],
                      pointers->totalDocLen / (float) pointers->totalDocs,
                      pointers->defaultIdf, (BM25Parameter*) scorer->parameters);
  } else if(scorer->function == DIRICHLET && scorer->phrase) {
    return dirichlet(tf, pointers->docLen->counter[docid],
                     pointers->defaultCf, pointers->totalDocLen,
                     (DirichletParameter*) scorer->parameters);
  }
}

#endif
