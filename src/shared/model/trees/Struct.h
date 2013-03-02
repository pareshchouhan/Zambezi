#ifndef STRUCT_H_GUARD
#define STRUCT_H_GUARD

#include<stdlib.h>

typedef struct Struct Struct;

struct Struct {
  Struct* right;
  Struct* left;
  int fid;
  unsigned long id;
  float threshold;
};

Struct* createNode(unsigned long id, int fid, float threshold) {
  Struct* node = (Struct*) malloc(sizeof(Struct));
  node->id = id;
  node->fid = fid;
  node->threshold = threshold;
  node->left = 0;
  node->right = 0;

  return node;
}

void destroyTree(Struct* node) {
  if(node->right != 0) {
    destroyTree(node->right);
    free(node->right);
    node->right = 0;
  }
  if(node->left != 0) {
    destroyTree(node->left);
    free(node->left);
    node->left = 0;
  }
}

Struct* addNode(Struct* node, unsigned long id, int leftChild, int featureId, float threshold) {
  if(leftChild) {
    node->left = createNode(id, featureId, threshold);
    return node->left;
  } else {
    node->right = createNode(id, featureId, threshold);
    return node->right;
  }
}

Struct* getLeaf(Struct* pThis, float* featureVector) {
  if(!pThis->left && !pThis->right) {
    return pThis;
  }
  if(featureVector[pThis->fid] <= pThis->threshold) {
    return getLeaf(pThis->left, featureVector);
  } else {
    return getLeaf(pThis->right, featureVector);
  }
}

#endif
