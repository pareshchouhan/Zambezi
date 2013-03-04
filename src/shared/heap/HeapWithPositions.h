#ifndef HEAP_WITH_POSITIONS_H_GUARD
#define HEAP_WITH_POSITIONS_H_GUARD

#include <stdlib.h>
#include <float.h>

typedef struct Candidate Candidate;
typedef struct HeapWithPositions HeapWithPositions;

struct Candidate {
  int docid;
  int** positions;
};

struct HeapWithPositions {
  Candidate** candidate;
  float* score;
  int size;
  int index;
  int qlen;
};

HeapWithPositions* initHeapWithPositions(int size, int qlen) {
  HeapWithPositions* heap = (HeapWithPositions*) malloc(sizeof(HeapWithPositions));
  heap->index = 0;
  heap->qlen = qlen;
  heap->size = size + 2;
  heap->candidate = (Candidate**) malloc(heap->size * sizeof(Candidate*));
  int i = 0;
  for(i = 0; i < heap->size; i++) {
    heap->candidate[i] = NULL;
  }
  heap->score = (float*) malloc(heap->size * sizeof(float));
  heap->score[0] = -FLT_MAX;
  return heap;
}

void destroyCandidate(Candidate* candidate, int qlen) {
  int j;
  for(j = 0; j < qlen; j++) {
    free(candidate->positions[j]);
  }
  free(candidate->positions);
  free(candidate);
}

void destroyHeapWithPositions(HeapWithPositions* heap, int candidates) {
  if(candidates) {
    int i;
    for(i = 0; i < heap->size; i++) {
      if(heap->candidate[i]) {
        destroyCandidate(heap->candidate[i], heap->qlen);
      }
    }
  }
  free(heap->candidate);
  free(heap->score);
  free(heap);
}

int isFullHeapWithPositions(HeapWithPositions* heap) {
  return heap->index >= heap->size - 2;
}

void deleteMinHeapWithPositions(HeapWithPositions* heap) {
  Candidate* minElement;
  Candidate* lastElement;
  int child,now;
  float lastScore;
  minElement = heap->candidate[1];
  heap->candidate[1] = NULL;
  lastScore = heap->score[heap->index];
  lastElement = heap->candidate[heap->index];
  heap->candidate[heap->index] = NULL;
  heap->index--;

  for(now = 1; now*2 <= heap->index; now = child) {
    child = now*2;
    if(child != heap->index && heap->score[child+1] < heap->score[child] ) {
      child++;
    }
    if(lastScore > heap->score[child]) {
      heap->candidate[now] = heap->candidate[child];
      heap->candidate[child] = NULL;
      heap->score[now] = heap->score[child];
    } else {
      break;
    }
  }
  heap->candidate[now] = lastElement;
  heap->score[now] = lastScore;
  destroyCandidate(minElement, heap->qlen);
}

void insertHeapWithPositions(HeapWithPositions* heap, Candidate* candidate, float score) {
  heap->index++;
  heap->candidate[heap->index] = candidate;
  heap->score[heap->index] = score;

  int now = heap->index;
  while(heap->score[now/2] > score) {
    heap->candidate[now] = heap->candidate[now/2];
    heap->candidate[now/2] = NULL;
    heap->score[now] = heap->score[now/2];
    now /= 2;
  }
  heap->candidate[now] = candidate;
  heap->score[now] = score;

  if(heap->index == heap->size - 1) {
    deleteMinHeapWithPositions(heap);
  }
}

float minScoreHeapWithPositions(HeapWithPositions* heap) {
  return heap->score[1];
}

Candidate* minCandidateHeapWithPositions(HeapWithPositions* heap) {
  return heap->candidate[1];
}
#endif
