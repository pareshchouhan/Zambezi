#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "dictionary/Dictionary.h"
#include "buffer/FixedIntCounter.h"
#include "buffer/FixedLongCounter.h"
#include "util/ParseCommandLine.h"
#include "PostingsPool.h"
#include "DocumentVector.h"
#include "Pointers.h"
#include "Config.h"

#define TERMINAL_DOCID -1

int main (int argc, char** args) {
  char* inputPath = getValueCL(argc, args, "-input");
  char* outputPath = getValueCL(argc, args, "-output");

  char dicPath[1024];
  strcpy(dicPath, inputPath);
  strcat(dicPath, "/");
  strcat(dicPath, DICTIONARY_FILE);
  FILE* fp = fopen(dicPath, "rb");
  Dictionary** dic = readDictionary(fp);
  fclose(fp);

  char pointerPath[1024];
  strcpy(pointerPath, inputPath);
  strcat(pointerPath, "/");
  strcat(pointerPath, POINTER_FILE);
  fp = fopen(pointerPath, "rb");
  Pointers* pointers = readPointers(fp);
  fclose(fp);

  //create a contiguous index
  char indexPath[1024];
  strcpy(indexPath, inputPath);
  strcat(indexPath, "/");
  strcat(indexPath, INDEX_FILE);
  fp = fopen(indexPath, "rb");

  int bloomEnabled;
  unsigned int nbHash, bitsPerElement;
  readBloomStats(fp, &bloomEnabled, &nbHash, &bitsPerElement);
  int reverse = readReverseFlag(fp);
  PostingsPool* contiguousPool = createPostingsPool(NUMBER_OF_POOLS, reverse, bloomEnabled,
                                                    nbHash, bitsPerElement);
  Pointers* contiguousPointers = createPointers(DEFAULT_VOCAB_SIZE);

  int term = -1;
  while((term = nextTerm(pointers, term)) != -1) {
    long pointer = getHeadPointer(pointers, term);
    long newPointer = readPostingsForTerm(contiguousPool, pointer, fp);
    setHeadPointer(contiguousPointers, term, newPointer);
    setDf(contiguousPointers, term, getDf(pointers, term));
    setMaxTf(contiguousPointers, term,
             getMaxTf(pointers, term),
             getMaxTfDocLen(pointers, term));
  }

  int docid = -1;
  while((docid = nextIndexFixedIntCounter(pointers->docLen, docid)) != -1) {
    setDocLen(contiguousPointers, docid, pointers->docLen->counter[docid]);
  }

  contiguousPointers->totalDocs = pointers->totalDocs;
  contiguousPointers->totalDocLen = pointers->totalDocLen;
  fclose(fp);
  //end sorting index

  //write output
  char odicPath[1024];
  strcpy(odicPath, outputPath);
  strcat(odicPath, "/");
  strcat(odicPath, DICTIONARY_FILE);

  fp = fopen(odicPath, "wb");
  writeDictionary(dic, fp);
  fclose(fp);

  char oindexPath[1024];
  strcpy(oindexPath, outputPath);
  strcat(oindexPath, "/");
  strcat(oindexPath, INDEX_FILE);

  fp = fopen(oindexPath, "wb");
  writePostingsPool(contiguousPool, fp);
  fclose(fp);

  char opointerPath[1024];
  strcpy(opointerPath, outputPath);
  strcat(opointerPath, "/");
  strcat(opointerPath, POINTER_FILE);

  fp = fopen(opointerPath, "wb");
  writePointers(contiguousPointers, fp);
  fclose(fp);

  // Copy document vectors
  char vectorsPath[1024];
  strcpy(vectorsPath, inputPath);
  strcat(vectorsPath, "/");
  strcat(vectorsPath, DOCUMENT_VECTOR_FILE);
  if(!access(vectorsPath, F_OK)) {
    fp = fopen(vectorsPath, "rb");
    DocumentVector* vectors = readDocumentVector(fp);
    fclose(fp);

    strcpy(vectorsPath, outputPath);
    strcat(vectorsPath, "/");
    strcat(vectorsPath, DOCUMENT_VECTOR_FILE);
    fp = fopen(vectorsPath, "wb");
    writeDocumentVector(vectors, fp);
    fclose(fp);
    destroyDocumentVector(vectors);
  }

  destroyDictionary(dic);
  destroyPostingsPool(contiguousPool);
  destroyPointers(pointers);
  destroyPointers(contiguousPointers);

  return 0;
}
