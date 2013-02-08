#ifndef TERM_FEATURE_H_GUARD
#define TERM_FEATURE_H_GUARD

#include "scorer/ScoringFunction.h"

float computeTermFeature(int** positions, int* query, int qlength, int docid,
                         Pointers* pointers, ScoringFunction* scorer) {
  float score = 0;
  int i;
  for(i = 0; i < qlength; i++) {
    score += computeTermScoringFunction(scorer, query[i], docid, positions[i][0], pointers);
  }
  return score;
}

#endif
