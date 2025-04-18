#ifndef RB_TREE
#define RB_TREE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"

enum Color { RED=0, BLACK };

enum Traversal {
	PREORDER=0,
	INORDER,
	POSTORDER
};


typedef struct Dtype { // abstract data type, representing pcb_t
    struct pcb_t *proc; // pointer to the process control block
    double key; // key for comparison
    int timestamp;
} Dtype;

typedef struct RBNode {
    Dtype *data;
    enum Color color;
    struct RBNode *left, *right, *parent;
} RBNode;

int compare(const Dtype *a, const Dtype *b);
Dtype *createDtype(struct pcb_t *proc, int timestamp);

// Rotation operations
RBNode *rotateLeft(RBNode *root, RBNode *x);
RBNode *rotateRight(RBNode *root, RBNode *y);

// Finding operation
RBNode *findNode(RBNode *root, Dtype *data);

// Replace u with v in the tree
void transplant(RBNode **root, RBNode *u, RBNode *v); 

// Minimum right subtree
RBNode *minimumRightSubTree(RBNode *node); 

// Insertion operations
void insertNode(RBNode **root, Dtype *data);
void insertFixup(RBNode **root, RBNode *z); // Fixup after insertion (recoloring and rotations)

// Deletion operations
void deleteNode(RBNode **root, Dtype *data);
void deleteFixup(RBNode **root, RBNode *x, RBNode *x_parent); // Fixup after deletion (recoloring and rotations)

// Free tree
void freeRBTree(RBNode *root);

// Traversal operations
void Traverse(RBNode *root, void (*visit)(RBNode *node), enum Traversal order);

// Getting minimum node
RBNode *getMinNode(RBNode *root);

#endif // RB_TREE