#ifndef BWAND_AND_H_GUARD
#define BWAND_AND_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "PostingsPool.h"

#define TERMINAL_DOCID -1

int* bwandAnd(PostingsPool* pool, long* startPointers,
           int len, int hits) {
  int* set = (int*) calloc(hits, sizeof(int));
  unsigned int* blockDocid = (unsigned int*) calloc(2 * BLOCK_SIZE, sizeof(unsigned int));
  unsigned int count;
  int posting;
  int i, j, iSet = 0;

  count = decompressDocidBlock(pool, blockDocid, startPointers[0]);
  posting = 0;

  while(1) {
    int pivot = blockDocid[posting++];

    int found = 1;
    for(i = 1; i < len; i++) {
      if(!containsDocid(pool, pivot, &startPointers[i])) {
        found = 0;
        break;
      }
    }

    if(found) {
      set[iSet++] = pivot;
      if(iSet >= hits) break;
    }

    if(posting == count) {
      startPointers[0] = nextPointer(pool, startPointers[0]);
      if(startPointers[0] == UNDEFINED_POINTER) {
        break;
      }
      memset(blockDocid, 0, BLOCK_SIZE * 2 * sizeof(unsigned int));
      count = decompressDocidBlock(pool, blockDocid, startPointers[0]);
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
