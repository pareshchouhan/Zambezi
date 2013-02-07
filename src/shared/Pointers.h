#ifndef POINTERS_H_GUARD
#define POINTERS_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "Config.h"

typedef struct Pointers Pointers;

struct Pointers {
  FixedIntCounter* df;
  FixedLongCounter* cf;
  FixedLongCounter* headPointers;
  FixedIntCounter* docLen;
  FixedIntCounter* maxTf;
  FixedIntCounter* maxTfDocLen;
  int totalDocs;
  unsigned long totalDocLen;

  // Do not store
  unsigned int defaultDf;
  unsigned long defaultCf;
  float defaultIdf;
};

Pointers* createPointers(int size) {
  Pointers* pointers = (Pointers*) malloc(sizeof(Pointers));
  pointers->df = createFixedIntCounter(size, 0);
  pointers->cf = createFixedLongCounter(size, 0);
  pointers->headPointers = createFixedLongCounter(size, UNDEFINED_POINTER);
  pointers->docLen = createFixedIntCounter(size, 0);
  pointers->maxTf = createFixedIntCounter(size, 0);
  pointers->maxTfDocLen = createFixedIntCounter(size, 0);
  pointers->totalDocs = 0;
  pointers->totalDocLen = 0;
  return pointers;
}

void destroyPointers(Pointers* pointers) {
  destroyFixedLongCounter(pointers->headPointers);
  destroyFixedIntCounter(pointers->df);
  destroyFixedLongCounter(pointers->cf);
  destroyFixedIntCounter(pointers->docLen);
  destroyFixedIntCounter(pointers->maxTf);
  destroyFixedIntCounter(pointers->maxTfDocLen);
}

int getDf(Pointers* pointers, int term) {
  return getFixedIntCounter(pointers->df, term);
}

void setDf(Pointers* pointers, int term, int df) {
  setFixedIntCounter(pointers->df, term, df);
}

long getCf(Pointers* pointers, int term) {
  return getFixedLongCounter(pointers->cf, term);
}

void setCf(Pointers* pointers, int term, long cf) {
  setFixedLongCounter(pointers->cf, term, cf);
}

int getDocLen(Pointers* pointers, int docid) {
  return getFixedIntCounter(pointers->docLen, docid);
}

void setDocLen(Pointers* pointers, int docid, int docLen) {
  setFixedIntCounter(pointers->docLen, docid, docLen);
}

int getMaxTf(Pointers* pointers, int term) {
  return getFixedIntCounter(pointers->maxTf, term);
}

int getMaxTfDocLen(Pointers* pointers, int term) {
  return getFixedIntCounter(pointers->maxTfDocLen, term);
}

void setMaxTf(Pointers* pointers, int term, int tf, int dl) {
  setFixedIntCounter(pointers->maxTf, term, tf);
  setFixedIntCounter(pointers->maxTfDocLen, term, dl);
}

long getHeadPointer(Pointers* pointers, int term) {
  return getFixedLongCounter(pointers->headPointers, term);
}

void setHeadPointer(Pointers* pointers, int term, long sp) {
  setFixedLongCounter(pointers->headPointers, term, sp);
}

int nextTerm(Pointers* pointers, int currentTermId) {
  return nextIndexFixedLongCounter(pointers->headPointers, currentTermId);
}

void updateDefaultValues(Pointers* pointers) {
  pointers->defaultDf = pointers->totalDocs / 100;
  pointers->defaultCf = pointers->defaultDf * 2;
  pointers->defaultIdf = (float) log((pointers->totalDocs - pointers->defaultDf + 0.5f) /
                                     (pointers->defaultDf + 0.5f));
}

void writePointers(Pointers* pointers, FILE* fp) {
  int size = sizeFixedLongCounter(pointers->headPointers);
  fwrite(&size, sizeof(unsigned int), 1, fp);
  int term = -1;
  while((term = nextIndexFixedLongCounter(pointers->headPointers, term)) != -1) {
    fwrite(&term, sizeof(int), 1, fp);
    fwrite(&pointers->df->counter[term], sizeof(int), 1, fp);
    fwrite(&pointers->cf->counter[term], sizeof(long), 1, fp);
    fwrite(&pointers->headPointers->counter[term], sizeof(long), 1, fp);
    fwrite(&pointers->maxTf->counter[term], sizeof(int), 1, fp);
    fwrite(&pointers->maxTfDocLen->counter[term], sizeof(int), 1, fp);
  }

  size = sizeFixedIntCounter(pointers->docLen);
  fwrite(&size, sizeof(unsigned int), 1, fp);

  while((term = nextIndexFixedIntCounter(pointers->docLen, term)) != -1) {
    fwrite(&term, sizeof(int), 1, fp);
    fwrite(&pointers->docLen->counter[term], sizeof(int), 1, fp);
  }

  fwrite(&pointers->totalDocs, sizeof(int), 1, fp);
  fwrite(&pointers->totalDocLen, sizeof(unsigned long), 1, fp);
}

Pointers* readPointers(FILE* fp) {
  Pointers* pointers = createPointers(DEFAULT_VOCAB_SIZE);

  unsigned int size = 0;
  fread(&size, sizeof(unsigned int), 1, fp);
  int i, term, value;
  long pointer, cf;
  for(i = 0; i < size; i++) {
    fread(&term, sizeof(int), 1, fp);
    fread(&value, sizeof(int), 1, fp);
    setFixedIntCounter(pointers->df, term, value);
    fread(&cf, sizeof(long), 1, fp);
    setFixedLongCounter(pointers->cf, term, cf);
    fread(&pointer, sizeof(long), 1, fp);
    setFixedLongCounter(pointers->headPointers, term, pointer);
    fread(&value, sizeof(int), 1, fp);
    setFixedIntCounter(pointers->maxTf, term, value);
    fread(&value, sizeof(int), 1, fp);
    setFixedIntCounter(pointers->maxTfDocLen, term, value);
  }

  fread(&size, sizeof(unsigned int), 1, fp);
  for(i = 0; i < size; i++) {
    fread(&term, sizeof(int), 1, fp);
    fread(&value, sizeof(int), 1, fp);
    setFixedIntCounter(pointers->docLen, term, value);
  }

  fread(&pointers->totalDocs, sizeof(int), 1, fp);
  fread(&pointers->totalDocLen, sizeof(unsigned long), 1, fp);

  updateDefaultValues(pointers);
  return pointers;
}

#endif
