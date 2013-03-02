#ifndef TREE_BUILDER_H_GUARD
#define TREE_BUILDER_H_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Vectorized.h"
#include "Struct.h"

void (*findLeaf[151])(int* leaves, float* features,
                      int numberOfFeatures, Node* nodes) =
{0, &findLeafDepth1, &findLeafDepth2, &findLeafDepth3, &findLeafDepth4,
 &findLeafDepth5, &findLeafDepth6, &findLeafDepth7, &findLeafDepth8,
 &findLeafDepth9, &findLeafDepth10, &findLeafDepth11, &findLeafDepth12,
 &findLeafDepth13, &findLeafDepth14, &findLeafDepth15, &findLeafDepth16,
 &findLeafDepth17, &findLeafDepth18, &findLeafDepth19, &findLeafDepth20,
 &findLeafDepth21, &findLeafDepth22, &findLeafDepth23, &findLeafDepth24,
 &findLeafDepth25, &findLeafDepth26, &findLeafDepth27, &findLeafDepth28,
 &findLeafDepth29, &findLeafDepth30, &findLeafDepth31, &findLeafDepth32,
 &findLeafDepth33, &findLeafDepth34, &findLeafDepth35, &findLeafDepth36,
 &findLeafDepth37, &findLeafDepth38, &findLeafDepth39, &findLeafDepth40,
 &findLeafDepth41, &findLeafDepth42, &findLeafDepth43, &findLeafDepth44,
 &findLeafDepth45, &findLeafDepth46, &findLeafDepth47, &findLeafDepth48,
 &findLeafDepth49, &findLeafDepth50, &findLeafDepth51, &findLeafDepth52,
 &findLeafDepth53, &findLeafDepth54, &findLeafDepth55, &findLeafDepth56,
 &findLeafDepth57, &findLeafDepth58, &findLeafDepth59, &findLeafDepth60,
 &findLeafDepth61, &findLeafDepth62, &findLeafDepth63, &findLeafDepth64,
 &findLeafDepth65, &findLeafDepth66, &findLeafDepth67, &findLeafDepth68,
 &findLeafDepth69, &findLeafDepth70, &findLeafDepth71, &findLeafDepth72,
 &findLeafDepth73, &findLeafDepth74, &findLeafDepth75, &findLeafDepth76,
 &findLeafDepth77, &findLeafDepth78, &findLeafDepth79, &findLeafDepth80,
 &findLeafDepth81, &findLeafDepth82, &findLeafDepth83, &findLeafDepth84,
 &findLeafDepth85, &findLeafDepth86, &findLeafDepth87, &findLeafDepth88,
 &findLeafDepth89, &findLeafDepth90, &findLeafDepth91, &findLeafDepth92,
 &findLeafDepth93, &findLeafDepth94, &findLeafDepth95, &findLeafDepth96,
 &findLeafDepth97, &findLeafDepth98, &findLeafDepth99, &findLeafDepth100,
 &findLeafDepth101, &findLeafDepth102, &findLeafDepth103, &findLeafDepth104,
 &findLeafDepth105, &findLeafDepth106, &findLeafDepth107, &findLeafDepth108,
 &findLeafDepth109, &findLeafDepth110, &findLeafDepth111, &findLeafDepth112,
 &findLeafDepth113, &findLeafDepth114, &findLeafDepth115, &findLeafDepth116,
 &findLeafDepth117, &findLeafDepth118, &findLeafDepth119, &findLeafDepth120,
 &findLeafDepth121, &findLeafDepth122, &findLeafDepth123, &findLeafDepth124,
 &findLeafDepth125, &findLeafDepth126, &findLeafDepth127, &findLeafDepth128,
 &findLeafDepth129, &findLeafDepth130, &findLeafDepth131, &findLeafDepth132,
 &findLeafDepth133, &findLeafDepth134, &findLeafDepth135, &findLeafDepth136,
 &findLeafDepth137, &findLeafDepth138, &findLeafDepth139, &findLeafDepth140,
 &findLeafDepth141, &findLeafDepth142, &findLeafDepth143, &findLeafDepth144,
 &findLeafDepth145, &findLeafDepth146, &findLeafDepth147, &findLeafDepth148,
 &findLeafDepth149, &findLeafDepth150};

typedef struct TreeModel TreeModel;
struct TreeModel {
  Node* nodes;
  long* nodeSizes;
  long* treeDepths;
  int nbTrees;
};

void destroyTreeModel(TreeModel* model) {
  free(model->nodes);
  free(model->nodeSizes);
  free(model->treeDepths);
  free(model);
  model = 0;
}

int createNodes(Struct* root, long i, Node* nodes) {
  nodes[i].fid = root->fid;
  nodes[i].theta = root->threshold;
  if(!root->left && !root->right) {
    nodes[i].children[0] = i;
    nodes[i].children[1] = i;
  } else {
    nodes[i].children[0] = i + 1;
    int last = createNodes(root->left, i + 1, nodes);
    nodes[i].children[1] = last + 1;
    i = createNodes(root->right, last + 1, nodes);
  }
  return i;
}

TreeModel* parseTrees(char* configFile) {
  FILE *fp = fopen(configFile, "r");
  int nbTrees;
  fscanf(fp, "%d", &nbTrees);

  long totalNodes = 0;
  long* nodeSizes = (long*) malloc(nbTrees * sizeof(long));
  long* treeDepths = (long*) malloc(nbTrees * sizeof(long));
  Node** nodes = (Node**) malloc(nbTrees * sizeof(Node*));

  // Read an ensemble
  int tindex = 0;
  int arraySize = 2 * MAX_LEAVES;
  for(tindex = 0; tindex < nbTrees; tindex++) {
    fscanf(fp, "%ld", &treeDepths[tindex]);
    Struct** pointers = (Struct**) malloc(arraySize * sizeof(Struct*));
    Struct* root;
    char text[20];
    long line = 0;
    fscanf(fp, "%s", text);
    while(strcmp(text, "end") != 0) {
      long id;
      fscanf(fp, "%ld", &id);

      if(strcmp(text, "root") == 0) {
        int fid;
        float threshold;
        fscanf(fp, "%d %f", &fid, &threshold);
        root = createNode(id, fid, threshold);
        pointers[line] = root;
      } else if(strcmp(text, "node") == 0) {
        int fid;
        long pid;
        float threshold;
        int leftChild = 0;
        fscanf(fp, "%ld %d %d %f", &pid, &fid, &leftChild, &threshold);
        int parentIndex = 0;
        for(parentIndex = 0; parentIndex < arraySize; parentIndex++) {
          if(pointers[parentIndex]->id == pid) {
            break;
          }
        }
        pointers[line] = addNode(pointers[parentIndex], id, leftChild, fid, threshold);
      } else if(strcmp(text, "leaf") == 0) {
        long pid;
        int leftChild = 0;
        float value;
        fscanf(fp, "%ld %d %f", &pid, &leftChild, &value);
        int parentIndex = 0;
        for(parentIndex = 0; parentIndex < arraySize; parentIndex++) {
          if(pointers[parentIndex]->id == pid) {
            break;
          }
        }
        addNode(pointers[parentIndex], id, leftChild, 0, value);
      }
      fscanf(fp, "%s", text);
      line++;
    }
    totalNodes += line;
    nodeSizes[tindex] = line;

    nodes[tindex] = (Node*) malloc(line * sizeof(Node));
    createNodes(root, 0, nodes[tindex]);
    destroyTree(root);
    free(root);
    root = 0;
    free(pointers);
  }
  fclose(fp);

  // Pack all trees into a single array, thus avoiding two-D arrays.
  Node* all_nodes = (Node*) malloc(totalNodes * sizeof(Node));
  int newIndex = 0;
  for(tindex = 0; tindex < nbTrees; tindex++) {
    int nsize = nodeSizes[tindex];
    nodeSizes[tindex] = newIndex;
    int telement;
    for(telement = 0; telement < nsize; telement++) {
      all_nodes[newIndex].fid = abs(nodes[tindex][telement].fid);
      all_nodes[newIndex].theta = nodes[tindex][telement].theta;
      all_nodes[newIndex].children[0] = nodes[tindex][telement].children[0];
      all_nodes[newIndex].children[1] = nodes[tindex][telement].children[1];
      newIndex++;
    }
  }

  for(tindex = 0; tindex < nbTrees; tindex++) {
    free(nodes[tindex]);
  }
  free(nodes);
  nodes = 0;

  TreeModel* model = malloc(sizeof(TreeModel));
  model->nodes = all_nodes;
  model->nodeSizes = nodeSizes;
  model->treeDepths = treeDepths;
  model->nbTrees = nbTrees;
  return model;
}

#endif
