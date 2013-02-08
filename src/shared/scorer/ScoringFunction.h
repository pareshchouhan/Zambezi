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
    BM25Parameter* param = (BM25Parameter*) scorer->parameters;
    return bm25(tf, pointers->df->counter[query], pointers->totalDocs,
                pointers->docLen->counter[docid],
                pointers->totalDocLen / (float) pointers->totalDocs,
                param->K1, param->B);
  } else if(scorer->function == DIRICHLET) {
    DirichletParameter* param = (DirichletParameter*) scorer->parameters;
    return dirichlet(tf, pointers->docLen->counter[docid],
                     pointers->cf->counter[query], pointers->totalDocLen,
                     param->MU);
  }
}

float computePhraseScoringFunction(ScoringFunction* scorer,
                                   int docid, int tf, Pointers* pointers) {
  if(scorer->function == BM25) {
    BM25Parameter* param = (BM25Parameter*) scorer->parameters;
    return bm25Phrase(tf, pointers->docLen->counter[docid],
                      pointers->totalDocLen / (float) pointers->totalDocs,
                      pointers->defaultIdf, param->K1, param->B);
  } else if(scorer->function == DIRICHLET && scorer->phrase) {
    DirichletParameter* param = (DirichletParameter*) scorer->parameters;
    return dirichlet(tf, pointers->docLen->counter[docid],
                     pointers->defaultCf, pointers->totalDocLen,
                     param->MU);
  }
}

#endif
