#ifndef FIXED_BUFFER_H_GUARD
#define FIXED_BUFFER_H_GUARD

#include <stdlib.h>
#include <string.h>

typedef struct FixedBuffer FixedBuffer;

struct FixedBuffer {
  int* buffer;
  unsigned int bufferSize;
};

FixedBuffer* createFixedBuffer(unsigned int initialSize) {
  FixedBuffer* buffer = (FixedBuffer*) malloc(sizeof(FixedBuffer));
  buffer->bufferSize = initialSize;
  buffer->buffer = (int*) calloc(initialSize, sizeof(int));
  return buffer;
}

void destroyFixedBuffer(FixedBuffer* buffer) {
  free(buffer->buffer);
  free(buffer);
}

void resetFixedBuffer(FixedBuffer* buffer) {
  memset(buffer->buffer, 0, buffer->bufferSize);
}

void expandFixedBuffer(FixedBuffer* buffer) {
  int* temp = (int*) realloc(buffer->buffer, buffer->bufferSize * 2 * sizeof(int));
  memset(&temp[buffer->bufferSize], 0, buffer->bufferSize * sizeof(int));
  buffer->bufferSize *= 2;
  buffer->buffer = temp;
}

void setFixedBuffer(FixedBuffer* buffer, unsigned int index, int value) {
  while(index >= buffer->bufferSize) {
    expandFixedBuffer(buffer);
  }
  buffer->buffer[index] = value;
}

#endif
