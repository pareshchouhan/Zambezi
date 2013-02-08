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

float _default_bm25tf(int tf, int docLen, float avgDocLen) {
  return ((1.0f + DEFAULT_K1) * tf) / (DEFAULT_K1 * (1.0f - DEFAULT_B + DEFAULT_B * docLen / avgDocLen) + tf);
}

float _default_bm25(int tf, int df, int numDocs, int docLen, float avgDocLen) {
  return _default_bm25tf(tf, docLen, avgDocLen) * idf(numDocs, df);
}

float bm25tf(int tf, int docLen, float avgDocLen, float K1, float B) {
  return ((1.0f + K1) * tf) / (K1 * (1.0f - B + B * docLen / avgDocLen) + tf);
}

float bm25(int tf, int df, int numDocs, int docLen, float avgDocLen, float K1, float B) {
  return bm25tf(tf, docLen, avgDocLen, K1, B) * idf(numDocs, df);
}

float bm25Phrase(int tf, int docLen, float avgDocLen, float defaultIdf, float K1, float B) {
  return bm25tf(tf, docLen, avgDocLen, K1, B) * defaultIdf;
}

#endif
