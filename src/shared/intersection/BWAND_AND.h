#ifndef BWAND_AND_H_GUARD
#define BWAND_AND_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SegmentPool.h"

#define TERMINAL_DOCID -1

int* bwandAnd(SegmentPool* pool, long* headPointers,
           int len, int hits) {
  int* set = (int*) calloc(hits, sizeof(int));
  unsigned int* blockDocid = (unsigned int*) calloc(2 * BLOCK_SIZE, sizeof(unsigned int));
  unsigned int count;
  int posting;
  int i, j, iSet = 0, left = 1;

  count = decompressDocidBlock(pool, blockDocid, headPointers[0]);
  posting = 0;

  while(left) {
    int pivot = blockDocid[posting++];

    int found = 1;
    for(i = 1; i < len; i++) {
      if(headPointers[i] == UNDEFINED_POINTER) {
        left = 0;
        found = 0;
        break;
      }
      if(!containsDocid(pool, pivot, &headPointers[i])) {
        found = 0;
        break;
      }
    }

    if(found) {
      set[iSet++] = pivot;
      if(iSet >= hits) break;
    }

    if(posting == count && left) {
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

  if(iSet < hits) {
    set[iSet] = TERMINAL_DOCID;
  }
  return set;
}

#endif
