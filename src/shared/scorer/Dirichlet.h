#ifndef DIRICHLET_H_GUARD
#define DIRICHLET_H_GUARD

#include <math.h>

#define DEFAULT_MU 1000.0f

typedef struct DirichletParameter DirichletParameter;

struct DirichletParameter {
  float MU;
};

float termBackgroundProb(long cf, long collectionLength) {
  return (float) (((double) cf) / collectionLength);
}

float dirichlet(int tf, int docLen, long cf, long collectionLength, float MU) {
  return (float) log(((float) tf + MU * termBackgroundProb(cf, collectionLength)) /
                     (docLen + MU));
}

float dirichletPhrase(int tf, int docLen, long defaultCf,
                      long collectionLength, float MU) {
  return dirichlet(tf, docLen, defaultCf, collectionLength, MU);
}

#endif
