#ifndef DYNAMIC_BUFFER_H_GUARD
#define DYNAMIC_BUFFER_H_GUARD

#include <stdlib.h>
#include <string.h>
#include "SegmentPool.h"
#include "Config.h"

typedef struct BufferMaps BufferMaps;

// Buffer maps
struct BufferMaps {
  // Docid buffer map
  unsigned int** docid;
  // Term frequency buffer map
  unsigned int** tf;
  // Term positions buffer map
  unsigned int** position;
  // Table of tail pointers for vocabulary terms
  unsigned long* tailPointer;
  // Lengths of docid/tf buffers
  unsigned int* valueLength;
  // Cursor in the docid/tf buffers
  unsigned int* valuePosition;
  // Lengths of term positions buffers
  unsigned int* pvalueLength;
  // Cursor in the term positions buffers
  unsigned int* pvaluePosition;

  // Current capacity (number of vocabulary terms)
  unsigned int capacity;
};

/**
 * Creates buffer maps.
 *
 * @param initialSize Initial capacity of buffer maps
 * @param positional Type of index (non-positional, docids and tf,
 *        or positional)
 */
BufferMaps* createBufferMaps(unsigned int initialSize,
                             int positional) {
  BufferMaps* buffer = (BufferMaps*)
    malloc(sizeof(BufferMaps));
  buffer->capacity = initialSize;
  buffer->docid = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  buffer->valueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->valuePosition = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  buffer->tailPointer = (unsigned long*) calloc(initialSize, sizeof(unsigned long));
  memset(buffer->tailPointer, UNDEFINED_POINTER,
         initialSize * sizeof(unsigned long));

  if(positional == TFONLY || positional == POSITIONAL) {
    buffer->tf = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  } else {
    buffer->tf = NULL;
  }

  if(positional == POSITIONAL) {
    buffer->position = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
    buffer->pvalueLength = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
    buffer->pvaluePosition = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  } else {
    buffer->position = NULL;
    buffer->pvalueLength = NULL;
    buffer->pvaluePosition = NULL;
  }
  return buffer;
}

void destroyBufferMaps(BufferMaps* buffer) {
  int i;
  if(buffer->position) {
    for(i = 0; i < buffer->capacity; i++) {
      if(buffer->docid[i]) {
        free(buffer->docid[i]);
        free(buffer->tf[i]);
        free(buffer->position[i]);
      }
    }
    free(buffer->tf);
    free(buffer->position);
    free(buffer->pvalueLength);
    free(buffer->pvaluePosition);
  } else if(buffer->tf) {
    for(i = 0; i < buffer->capacity; i++) {
      if(buffer->docid[i]) {
        free(buffer->docid[i]);
        free(buffer->tf[i]);
      }
    }
    free(buffer->tf);
  } else {
    for(i = 0; i < buffer->capacity; i++) {
      if(buffer->docid[i]) {
        free(buffer->docid[i]);
      }
    }
  }

  free(buffer->docid);
  free(buffer->valueLength);
  free(buffer->valuePosition);
  free(buffer->tailPointer);
  free(buffer);
}

/**
 * Expand buffer maps' capacities by a factor of 2
 */
void expandBufferMaps(BufferMaps* buffer) {
  unsigned int** tempDocid = (unsigned int**) realloc(buffer->docid,
      buffer->capacity * 2 * sizeof(unsigned int*));
  unsigned int* tempValueLength = (unsigned int*) realloc(buffer->valueLength,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned int* tempValuePosition = (unsigned int*) realloc(buffer->valuePosition,
      buffer->capacity * 2 * sizeof(unsigned int));
  unsigned long* tempTailPointer = (unsigned long*) realloc(buffer->tailPointer,
      buffer->capacity * 2 * sizeof(unsigned long));

  int i;
  for(i = buffer->capacity; i < buffer->capacity * 2; i++) {
    tempDocid[i] = NULL;
    tempValueLength[i] = 0;
    tempValuePosition[i] = 0;
    tempTailPointer[i] = UNDEFINED_POINTER;
  }

  buffer->docid = tempDocid;
  buffer->valueLength = tempValueLength;
  buffer->valuePosition = tempValuePosition;
  buffer->tailPointer = tempTailPointer;

  if(buffer->position) {
    unsigned int** tempPosition = (unsigned int**) realloc(buffer->position,
        buffer->capacity * 2 * sizeof(unsigned int*));
    unsigned int* tempPValueLength = (unsigned int*) realloc(buffer->pvalueLength,
        buffer->capacity * 2 * sizeof(unsigned int));
    unsigned int* tempPValuePosition = (unsigned int*) realloc(buffer->pvaluePosition,
        buffer->capacity * 2 * sizeof(unsigned int));

    int j;
    for(j = buffer->capacity; j < buffer->capacity * 2; j++) {
      tempPosition[j] = NULL;
      tempPValueLength[j] = 0;
      tempPValuePosition[j] = 0;
    }

    buffer->position = tempPosition;
    buffer->pvalueLength = tempPValueLength;
    buffer->pvaluePosition = tempPValuePosition;
  }

  if(buffer->tf) {
    unsigned int** tempTf = (unsigned int**) realloc(buffer->tf,
        buffer->capacity * 2 * sizeof(unsigned int*));

    int j;
    for(j = buffer->capacity; j < buffer->capacity * 2; j++) {
      tempTf[j] = NULL;
    }
    buffer->tf = tempTf;
  }

  buffer->capacity *= 2;
}

/**
 * Whether buffer maps contain a buffer for a given vocabulary term
 *
 * @param buffer Buffer maps
 * @param k Term id
 */
int containsKeyBufferMaps(BufferMaps* buffer, int k) {
  return buffer->docid[k] != NULL;
}

int* getDocidBufferMaps(BufferMaps* buffer, int k) {
  if(k >= buffer->capacity) {
    expandBufferMaps(buffer);
  }

  return buffer->docid[k];
}

int* getTfBufferMaps(BufferMaps* buffer, int k) {
  if(k >= buffer->capacity) {
    expandBufferMaps(buffer);
  }

  return buffer->tf[k];
}

int* getPositionBufferMaps(BufferMaps* buffer, int k) {
  if(k >= buffer->capacity) {
    expandBufferMaps(buffer);
  }

  return buffer->position[k];
}

/**
 * An iterator that goes through the vocabulary terms,
 * and return the index of the next buffer whose length is more
 * than a given threshold.
 *
 * @param buffer Buffer map
 * @param pos Current index
 * @param minLength Minimum length to consider
 */
int nextIndexBufferMaps(BufferMaps* buffer, int pos, int minLength) {
  pos++;
  if(pos >= buffer->capacity) {
    return -1;
  }
  while(buffer->valueLength[pos] < minLength) {
    pos++;
    if(pos >= buffer->capacity) {
      return -1;
    }
  }
  return pos;
}

#endif
