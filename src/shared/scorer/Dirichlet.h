#ifndef DIRICHLET_H_GUARD
#define DIRICHLET_H_GUARD

#include <math.h>

#define DEFAULT_MU 1000.0f

typedef struct DirichletParameter DirichletParameter;

struct DirichletParameter {
  float MU;
};

float termBackgroundProb(long cf, long collectionLength) {
  return (float) (cf / collectionLength);
}

float dirichlet(int tf, int docLen, long cf, long collectionLength,
                DirichletParameter* parameters) {
  return (float) log(((float) tf + parameters->MU * termBackgroundProb(cf, collectionLength)) /
                     (docLen + parameters->MU));
}

float dirichletPhrase(int tf, int docLen, long defaultCf,
                      long collectionLength, DirichletParameter* parameters) {
  return dirichlet(tf, docLen, defaultCf, collectionLength, parameters);
}

#endif
