#ifndef BWAND_OR_H_GUARD
#define BWAND_OR_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "heap/Heap.h"
#include "SegmentPool.h"

#define TERMINAL_DOCID -1

int* bwandOr(SegmentPool* pool, long* headPointers,
             float* UB, int len, int hits, float** scores) {
  Heap* elements = initHeap(hits);
  unsigned int* blockDocid = (unsigned int*) calloc(2 * BLOCK_SIZE, sizeof(unsigned int));
  unsigned int count;
  int posting;
  float threshold = 0;
  int i, j;
  float sumOfUB = 0;
  for(i = 0; i < len; i++) {
    sumOfUB += UB[i];
  }

  count = decompressDocidBlock(pool, blockDocid, headPointers[0]);
  posting = 0;
  if(UB[0] <= threshold) {
    threshold = UB[0] - 1;
  }

  while(1) {
    int pivot = blockDocid[posting++];

    float score = UB[0];
    for(i = 1; i < len; i++) {
      if(containsDocid(pool, pivot, &headPointers[i])) {
        score += UB[i];
      }
    }

    if(score > threshold) {
      insertHeap(elements, pivot, score);
      if(isFullHeap(elements)) {
        threshold = minScoreHeap(elements);
        if(threshold == sumOfUB) {
          break;
        }
      }
    }

    if(posting == count) {
      headPointers[0] = nextPointer(pool, headPointers[0]);
      if(headPointers[0] == UNDEFINED_POINTER) {
        break;
      }
      memset(blockDocid, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      count = decompressDocidBlock(pool, blockDocid, headPointers[0]);
      posting = 0;
    }
  }

  // Free the allocated memory
  free(blockDocid);

  int* set = (int*) calloc(elements->index + 1, sizeof(int));
  memcpy(set, &elements->docid[1], elements->index * sizeof(int));
  memcpy(*scores, &elements->score[1], elements->index *sizeof(float));
  if(!isFullHeap(elements)) {
    set[elements->index] = TERMINAL_DOCID;
  }
  destroyHeap(elements);
  return set;
}

#endif
