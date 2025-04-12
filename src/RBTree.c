#include "../include/RBTree.h"

/*
* Function: compare
* -------------------
* Compares two Dtype objects based on their key values.
*
* a: pointer to the first Dtype object
* b: pointer to the second Dtype object
*
* returns: -1 if a < b, 1 if a > b, 0 if a == b
*/
int compare(const Dtype *a, const Dtype *b) {
    if (a->key < b->key) return -1;
    if (a->key > b->key) return 1;
    return 0;
}

Dtype *createDtype(struct pcb_t *proc) {
    Dtype *data = (Dtype *)malloc(sizeof(Dtype));
    if (!data) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    data->proc = proc;
#ifdef CFS_SCHED
    data->key = proc->vruntime;
#else
    data->key = proc->prio; 
#endif
    return data;
}

// Rotation operations
RBNode *rotateLeft(RBNode *root, RBNode *x) {
    RBNode *y = x->right;
    x->right = y->left;

    if (y->left != NULL) {
        y->left->parent = x;
    }

    y->parent = x->parent;

    if (x->parent == NULL) {
        root = y; 
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
    return root;
}

RBNode *rotateRight(RBNode *root, RBNode *y) {
    RBNode *x = y->left;
    y->left = x->right;

    if (x->right != NULL) {
        x->right->parent = y;
    }

    x->parent = y->parent;

    if (y->parent == NULL) {
        root = x; 
    } else if (y == y->parent->left) {
        y->parent->left = x;
    } else {
        y->parent->right = x;
    }

    x->right = y;
    y->parent = x;
    return root;
}


// Finding operation
RBNode *findNode(RBNode *root, Dtype *data) {
    RBNode *iter = root;
    while (iter && compare(data, iter->data) != 0) {
        if (compare(data, iter->data) == -1) {
            iter = iter->left;
        } else {
            iter = iter->right;
        }
    }
    return iter;
}


// Transplant operation
// Replace u with v in the tree
void transplant(RBNode **root, RBNode *u, RBNode *v) {
    if (u->parent == NULL) { // u is root
        *root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v != NULL) {
        v->parent = u->parent;
    } 
}


// Minimum right subtree
RBNode *minimumRightSubTree(RBNode *node) {
    node = node->right;
    while (node && node->left != NULL) {
        node = node->left;
    }
    return node;
}


// Insertions operations
void insertNode(RBNode **root, Dtype *data) {
    RBNode *new_node = (RBNode *)malloc(sizeof(RBNode));
    if (!new_node) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    new_node->data = data;
    new_node->color = RED;
    new_node->left = new_node->right = new_node->parent = NULL;

    if (*root == NULL) {
        *root = new_node;
        (*root)->color = BLACK;
        return;
    }

    RBNode *iter = *root;
    RBNode *parent = NULL;
    while (iter != NULL) {
        parent = iter;
        if (compare(new_node->data, iter->data) == -1) {
            iter = iter->left;
        } else {
            iter = iter->right;
        }
    }
    new_node->parent = parent;
    if (compare(new_node->data, new_node->parent->data) == -1) 
        new_node->parent->left = new_node;
    else 
        new_node->parent->right = new_node;

    insertFixup(root, new_node);
}

void insertFixup(RBNode **root, RBNode *z) {
    if (z->parent == NULL) 
        return;

    while (z->parent && z->parent->color == RED) {
        RBNode *grandParent = z->parent->parent;
        RBNode *parent = z->parent;

        // prarent of new_node is the root
        if (!grandParent) {
            z->parent->color = BLACK;
            break;
        } else {
            if (grandParent->left == parent) {
                if (grandParent->right && grandParent->right->color == RED) {
                    grandParent->right->color = grandParent->left->color = BLACK;
                    grandParent->color = RED;
                    z = grandParent;
                } else {
                    if (parent->right == z) {
                        z = parent;
                        *root = rotateLeft(*root, z);
                    } else {
                        parent->color = BLACK;
                        grandParent->color = RED;
                        *root = rotateRight(*root, grandParent);
                    }
                }
            } else {
                if (grandParent->left && grandParent->left->color == RED) {
                    grandParent->right->color = grandParent->left->color = BLACK;
                    grandParent->color = RED;
                    z = grandParent;
                } else {
                    if (parent->left == z) {
                        z = parent;
                        *root = rotateRight(*root, z);
                    } else {
                        parent->color = BLACK;
                        grandParent->color = RED;
                        *root = rotateLeft(*root, grandParent);
                    }
                }
            }
        }
    }
    (*root)->color = BLACK;
}


// Deletion operations
void deleteNode(RBNode **root, Dtype *data) {
    RBNode *del_node = findNode(*root, data);
    if (!del_node) {
        fprintf(stderr, "Node with key %d not found.\n", data->key);
        return;
    }
    
    RBNode *y = del_node;  // Node to be removed from the tree
    RBNode *x = NULL;      // Node to replace y
    RBNode *x_parent = NULL;
    enum Color y_original_color = y->color;
    
    if (del_node->left == NULL) {
        x = del_node->right;
        x_parent = del_node->parent;
        transplant(root, del_node, del_node->right);
        
        // Free memory
        Dtype *temp = del_node->data;
        del_node->data = NULL;
        free(temp);
        free(del_node);
    } else if (del_node->right == NULL) {
        x = del_node->left;
        x_parent = del_node->parent;
        transplant(root, del_node, del_node->left);
        
        // Free memory
        Dtype *temp = del_node->data;
        del_node->data = NULL;
        free(temp);
        free(del_node);
    } else {
        y = minimumRightSubTree(del_node);
        y_original_color = y->color;
        x = y->right;
        
        if (y->parent == del_node) {
            x_parent = y;
            if (x) x->parent = y;
        } 
        else {
            x_parent = y->parent;
            transplant(root, y, y->right);
            
            y->right = del_node->right;
            if (del_node->right) del_node->right->parent = y;
        }
        
        transplant(root, del_node, y);
        
        y->left = del_node->left;
        if (del_node->left) del_node->left->parent = y;
        y->color = del_node->color;
        
        // Free memory
        Dtype *temp = del_node->data;
        del_node->data = NULL;
        free(temp);
        free(del_node);
    }
    
    // Fix RB tree properties if needed
    if (y_original_color == BLACK) {
        deleteFixup(root, x, x_parent);
    }
}

void deleteFixup(RBNode **root, RBNode *x, RBNode *x_parent) {
    while ((x == NULL || x->color == BLACK) && x != *root) {
        if (x == NULL || (x_parent && x == x_parent->left)) {
            RBNode *w = x_parent ? x_parent->right : NULL;
            
            if (w && w->color == RED) {
                // Case 1: sibling is red
                w->color = BLACK;
                x_parent->color = RED;
                *root = rotateLeft(*root, x_parent);
                w = x_parent->right;
            }
            
            if (w && (w->left == NULL || w->left->color == BLACK) && 
                     (w->right == NULL || w->right->color == BLACK)) {
                // Case 2: sibling is black and both its children are black
                w->color = RED;
                x = x_parent;
                x_parent = x ? x->parent : NULL;
            } else if (w) {
                if (w->right == NULL || w->right->color == BLACK) {
                    // Case 3: sibling is black, left child is red, right child is black
                    if (w->left) w->left->color = BLACK;
                    w->color = RED;
                    *root = rotateRight(*root, w);
                    w = x_parent->right;
                }
                
                // Case 4: sibling is black, right child is red
                w->color = x_parent->color;
                x_parent->color = BLACK;
                if (w->right) w->right->color = BLACK;
                *root = rotateLeft(*root, x_parent);
                x = *root; // This forces termination of the loop
                x_parent = NULL;
            } else {
                // No sibling, move up the tree
                x = x_parent;
                x_parent = x ? x->parent : NULL;
            }
        } else {
            // Mirror image cases for right child
            RBNode *w = x_parent ? x_parent->left : NULL;
            
            if (w && w->color == RED) {
                w->color = BLACK;
                x_parent->color = RED;
                *root = rotateRight(*root, x_parent);
                w = x_parent->left;
            }
            
            if (w && (w->right == NULL || w->right->color == BLACK) && 
                     (w->left == NULL || w->left->color == BLACK)) {
                w->color = RED;
                x = x_parent;
                x_parent = x ? x->parent : NULL;
            } else if (w) {
                if (w->left == NULL || w->left->color == BLACK) {
                    if (w->right) w->right->color = BLACK;
                    w->color = RED;
                    *root = rotateLeft(*root, w);
                    w = x_parent->left;
                }
                
                w->color = x_parent->color;
                x_parent->color = BLACK;
                if (w->left) w->left->color = BLACK;
                *root = rotateRight(*root, x_parent);
                x = *root;
                x_parent = NULL;
            } else {
                x = x_parent;
                x_parent = x ? x->parent : NULL;
            }
        }
    }
    
    if (x) x->color = BLACK;
}


// Free tree
void freeRBTree(RBNode *root) {
    if (root) {
        freeRBTree(root->left);
        freeRBTree(root->right);
        free(root->data); // Free the data in the node
        free(root); // Free the node itself
    }
}


// Traversal operations
void traverse(RBNode *root, void (*visit)(RBNode *node), enum Traversal order) {
    if (root == NULL) return;
    
    if (order == PREORDER) {
        visit(root);
        traverse(root->left, visit, order);
        traverse(root->right, visit, order);
    } else if (order == INORDER) {
        traverse(root->left, visit, order);
        visit(root);
        traverse(root->right, visit, order);
    } else if (order == POSTORDER) {
        traverse(root->left, visit, order);
        traverse(root->right, visit, order);
        visit(root);
    }
}

void printNode(RBNode *node) {
    if (!node) return;
    printf("%d", node->data->key);
    printf("%s", node->color == RED ? "R" : "B");
    printf(" ");
}


// Getting minimum node
RBNode *getMinNode(RBNode *root) {
    if (root == NULL) return NULL;
    while (root->left != NULL) {
        root = root->left;
    }
    return root;
}