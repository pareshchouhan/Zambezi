#ifndef DOCUMENT_VECTOR_H_GUARD
#define DOCUMENT_VECTOR_H_GUARD

#include <stdlib.h>
#include <stdio.h>
#include "Config.h"
#include "buffer/FixedIntCounter.h"
#include "pfordelta/opt_p4.h"

typedef struct DocumentVector DocumentVector;

struct DocumentVector {
  unsigned int** document;
  unsigned int* length;
  unsigned int capacity;
};

void writeDocumentVector(DocumentVector* vectors, FILE* fp) {
  fwrite(&vectors->capacity, sizeof(unsigned int), 1, fp);
  int i;
  for(i = 0; i < vectors->capacity; i++) {
    if(vectors->document) {
      fwrite(&i, sizeof(int), 1, fp);
      fwrite(&vectors->length[i], sizeof(int), 1, fp);
      fwrite(vectors->document[i], sizeof(int), vectors->length[i], fp);
    }
  }
  i = -1;
  fwrite(&i, sizeof(int), 1, fp);
}

DocumentVector* readDocumentVector(FILE* fp) {
  DocumentVector* vectors = (DocumentVector*) malloc(sizeof(DocumentVector));
  fread(&vectors->capacity, sizeof(unsigned int), 1, fp);
  vectors->document = (unsigned int**) calloc(vectors->capacity, sizeof(unsigned int*));
  vectors->length = (unsigned int*) calloc(vectors->capacity, sizeof(unsigned int));

  int i;
  fread(&i, sizeof(int), 1, fp);
  while(i >= 0) {
    fread(&vectors->length[i], sizeof(unsigned int), 1, fp);
    vectors->document[i] = (unsigned int*) calloc(vectors->length[i], sizeof(unsigned int));
    fread(vectors->document[i], sizeof(unsigned int), vectors->length[i], fp);
    fread(&i, sizeof(int), 1, fp);
  }
  return vectors;
}

DocumentVector* createDocumentVector(unsigned int initialSize) {
  DocumentVector* vectors = (DocumentVector*)
    malloc(sizeof(DocumentVector));
  vectors->capacity = initialSize;
  vectors->length = (unsigned int*) calloc(initialSize, sizeof(unsigned int));
  vectors->document = (unsigned int**) calloc(initialSize, sizeof(unsigned int*));
  int i;
  return vectors;
}

void destroyDocumentVector(DocumentVector* vectors) {
  int i;
  for(i = 0; i < vectors->capacity; i++) {
    if(vectors->document[i]) {
      free(vectors->document[i]);
    }
  }
  free(vectors->document);
  free(vectors->length);
  free(vectors);
}

void expandDocumentVector(DocumentVector* vectors) {
  unsigned int** tempDocument = (unsigned int**) realloc(vectors->document,
      vectors->capacity * 2 * sizeof(unsigned int*));
  unsigned int* tempLength = (unsigned int*) realloc(vectors->length,
      vectors->capacity * 2 * sizeof(unsigned int));
  int i;
  for(i = vectors->capacity; i < vectors->capacity * 2; i++) {
    tempDocument[i] = NULL;
    tempLength = 0;
  }
  vectors->document = tempDocument;
  vectors->length = tempLength;
  vectors->capacity *= 2;
}

int containsDocumentVector(DocumentVector* vectors, int k) {
  return vectors->document[k] != NULL;
}

void getDocumentVector(DocumentVector* vectors, unsigned int* document,
                      unsigned int length, int k) {
  if(k >= vectors->capacity || !vectors->document[k]) {
    document = NULL;
    return;
  }
  unsigned int aux[length * 2];
  detailed_p4_decode(document, vectors->document[k], aux, 0, 0);
}

void addDocumentVector(DocumentVector* vectors, unsigned int * document,
                       unsigned int length, int k) {
  if(k >= vectors->capacity) {
    expandDocumentVector(vectors);
  }

  unsigned int* block = (unsigned int*) calloc(length*2, sizeof(unsigned int));
  vectors->length[k] = OPT4(document, length, block, 1);
  vectors->document[k] = (unsigned int*) calloc(vectors->length[k], sizeof(unsigned int));
  int i;
  for(i = 0; i < vectors->length[k]; i++) {
    vectors->document[k][i] = block[i];
  }
  free(block);
}

#endif
