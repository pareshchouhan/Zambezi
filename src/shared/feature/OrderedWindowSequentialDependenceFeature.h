#ifndef ORDERED_WINDOW_SEQUENTIAL_DEPENDENCE_H_GUARD
#define ORDERED_WINDOW_SEQUENTIAL_DEPENDENCE_H_GUARD

#include <stdlib.h>
#include "scorer/ScoringFunction.h"

int* countOD(int** positions, int qlength, int gap) {
  int* tf = (int*) calloc(qlength - 1, sizeof(int));
  int i, j, k;
  for(i = 0; i < qlength - 1; i++) {
    int* p = &positions[i][1];
    int* pn = &positions[i + 1][1];

    for(j = 0; j < positions[i][0]; j++) {
      for(k = 0; k < positions[i + 1][0]; k++) {
        if(pn[k] > p[j] && (pn[k] - p[j] - 1) <= gap) {
          tf[i]++;
          break;
        }
      }
    }
  }
  return tf;
}

float computeOrderedWindowSDFeature(int** positions, int* query, int qlength, int docid,
                                    Pointers* pointers, ScoringFunction* scorer) {
  if(qlength == 1) {
    return 0;
  }

  int* tf = countOD(positions, qlength, scorer->phrase);
  float score = 0;
  int i;
  for(i = 0; i < qlength - 1; i++) {
    score += computePhraseScoringFunction(scorer, docid, tf[i], pointers);
  }
  free(tf);
  return score;
}

#endif
