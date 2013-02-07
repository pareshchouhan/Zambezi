#ifndef BM25_H_GUARD
#define BM25_H_GUARD

#include <math.h>

#define DEFAULT_K1 0.5f
#define DEFAULT_B 0.3f

typedef struct BM25Parameter BM25Parameter;

struct BM25Parameter {
  float K1;
  float B;
};

float idf(int numDocs, int df) {
  return (float) log(((float) numDocs - (float) df + 0.5f)
                     / ((float) df + 0.5f));
}

float bm25tf(int tf, int docLen, float avgDocLen, float K1, float B) {
  return ((1.0f + K1) * tf) / (K1 * (1.0f - B + B * docLen / avgDocLen) + tf);
}

float bm25(int tf, int df, int numDocs, int docLen, float avgDocLen, BM25Parameter* parameters) {
  return bm25tf(tf, docLen, avgDocLen, parameters->K1, parameters->B) * idf(numDocs, df);
}

float bm25Phrase(int tf, int docLen, float avgDocLen, float defaultIdf, BM25Parameter* parameters) {
  return bm25tf(tf, docLen, avgDocLen, parameters->K1, parameters->B) * defaultIdf;
}

#endif
