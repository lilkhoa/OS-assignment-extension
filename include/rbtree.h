#ifndef RBTREE_H
#define RBTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"

typedef enum rb_color {
    RED,
    BLACK
} rb_color;

typedef enum {
	PREORDER=0,
	INORDER,
	POSTORDER
} Traversal;

typedef struct{
    int key;
    int timestamp;
    struct pcb_t *proc; // pointer to the pcb_t struct
} CustomData;      //place hodler for custom pcb_t

typedef struct rb_node {
    CustomData* data;
    rb_color color;
    struct rb_node *left, *right, *parent;
} rb_node;

typedef struct rbtree{
    rb_node* root;
    int count;
}rbtree;

typedef struct rb_queue_node {  //for debugging
    rb_node *data;
    int level;  
    struct rb_queue_node *next;
} rb_queue_node;

typedef struct rb_queue {    //for debugging
    rb_queue_node *front, *rear;
} rb_queue;

int cmp(CustomData* a, CustomData* b);  //cmp key feature of 2 obj
rb_node* create_node(CustomData* data);
CustomData* createData(struct pcb_t *proc, int timestamp);
void rotate_left(rb_node **root, rb_node *x);
void rotate_right(rb_node **root, rb_node *y);
void fix_insert(rb_node **root, rb_node *z);
void insert(rbtree *tree, CustomData* data);
void insert_node(rb_node **root, CustomData* data);
rb_node* get_min_node(rb_node *root);
rb_node* find_node(rb_node *root, CustomData* data);
rb_node* find_rec(rb_node* rb_node, CustomData* data);
rb_node* min_right_tree(rb_node *rb_node);
void fix_delete(rb_node **root, rb_node *x, rb_node *parent_x, bool is_left);
void delete_node(rb_node **root, CustomData* data);
void deletion(rbtree *tree, CustomData* data);
// void in_order_traversal(rb_node *root);                    //for debugging
// rb_queue* create_queue();                                  //for debugging
// void enqueue(rb_queue *queue, rb_node *data, int level);   //for debugging
// rb_queue_node* dequeue(rb_queue *queue);                   //for debugging
// bool is_queue_empty(rb_queue *queue);                      //for debugging
// void level_order_traversal(rb_node *root);                 //for debugging
void free_node(rb_node *root);
void free_tree(rbtree *tree);
void traverse(rb_node *root, void (*visit)(rb_node *node), Traversal order);

#endif