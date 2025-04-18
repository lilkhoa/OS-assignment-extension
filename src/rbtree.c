#include "../include/rbtree.h"


int cmp(CustomData* a, CustomData* b){
    if(a->key < b->key) return -1;
    if(a->key == b->key) return 0;
    return 1;
}
rb_node* create_node(CustomData* data) {
    rb_node *newNode = (rb_node*)malloc(sizeof(rb_node));
    if (newNode == NULL) {
        exit(1);
    }
    
    newNode->data = data;
    newNode->color = RED; 
    newNode->left = newNode->right = newNode->parent = NULL;
    return newNode;
}

void rotate_left(rb_node **root, rb_node *x) {
    rb_node *y = x->right;
    x->right = y->left;
    
    if (y->left != NULL)
        y->left->parent = x;
    
    y->parent = x->parent;
    
    if (x->parent == NULL)
        *root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    
    y->left = x;
    x->parent = y;
}

void rotate_right(rb_node **root, rb_node *y) {
    rb_node *x = y->left;
    y->left = x->right;
    
    if (x->right != NULL)
        x->right->parent = y;
    
    x->parent = y->parent;
    
    if (y->parent == NULL)
        *root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
    
    x->right = y;
    y->parent = x;
}

rb_node* get_min_node(rb_node* root){
    if(!root) return NULL;
    rb_node* x = root;
    while(x->left){
        x = x->left;
    }
    return x;
}

// Fix tree properties after insertion
void fix_insert(rb_node **root, rb_node *z) {
    rb_node *grandparent = NULL;
    rb_node *uncle = NULL;
    
    while ((z != *root) && (z->color == RED) && (z->parent->color == RED)) {
        grandparent = z->parent->parent;
        
        if (z->parent == grandparent->left) {
            uncle = grandparent->right;
            
            if (uncle != NULL && uncle->color == RED) {
                grandparent->color = RED;
                z->parent->color = BLACK;
                uncle->color = BLACK;
                z = grandparent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotate_left(root, z);
                }
                z->parent->color = BLACK;
                grandparent->color = RED;
                rotate_right(root, grandparent);
            }
        }
        else {
            uncle = grandparent->left;
            if (uncle != NULL && uncle->color == RED) {
                grandparent->color = RED;
                z->parent->color = BLACK;
                uncle->color = BLACK;
                z = grandparent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotate_right(root, z);
                }
                z->parent->color = BLACK;
                grandparent->color = RED;
                rotate_left(root, grandparent);
            }
        }
    }
    (*root)->color = BLACK;
}

void insert(rbtree *tree, CustomData* data){
    insert_node(&(tree->root),data);
    tree->count++;
}
void insert_node(rb_node **root, CustomData* data) {
    rb_node *z = create_node(data);
    
    rb_node *y = NULL;
    rb_node *x = *root;

    while (x != NULL) {
        y = x;
        if (cmp(z->data,x->data) < 0)
            x = x->left;
        else 
            x = x->right;
    }
    
    z->parent = y;
    
    if (y == NULL)
        *root = z;
    else if (cmp(z->data, y->data) < 0)
        y->left = z;
    else
        y->right = z;
    
    fix_insert(root, z);
}


rb_node* find_rec(rb_node *rb_node, CustomData* data) {
    if(!rb_node) return NULL;
    if(data->key > rb_node->data->key)
        return find_rec(rb_node->right,data);
    if(data->key < rb_node->data->key)
        return find_rec(rb_node->left,data);
    if(data->timestamp == rb_node->data->timestamp) return rb_node; //unique feature cmp
    if(rb_node->left) return find_rec(rb_node->left,data);
    if(rb_node->right) return find_rec(rb_node->right,data);
    return NULL;
}

rb_node* find_node(rb_node *root, CustomData* data) {
    if(root){
        return find_rec(root,data);
    }
    return NULL;
}

rb_node* min_right_tree(rb_node *rb_node) {
    if (rb_node->right != NULL) {
        rb_node = rb_node->right;
        while (rb_node->left != NULL)
            rb_node = rb_node->left;
        return rb_node;
    }
    return NULL;
}

// Fix tree properties after deletion
void fix_delete(rb_node **root, rb_node *x, rb_node *parent_x, bool is_left) {
    while (x != *root && (x == NULL || x->color == BLACK)) {
        if (is_left) {
            rb_node *sibling = parent_x->right;
            if (sibling->color == RED) {
                sibling->color = BLACK;
                parent_x->color = RED;
                rotate_left(root, parent_x);
                sibling = parent_x->right;
            }
            if ((sibling->left == NULL || sibling->left->color == BLACK) &&
                (sibling->right == NULL || sibling->right->color == BLACK)) {
                sibling->color = RED;
                x = parent_x;
                if (x->parent != NULL)
                    is_left = (x == x->parent->left);
                parent_x = x->parent;
            } else {
                if (sibling->right == NULL || sibling->right->color == BLACK) {
                    if (sibling->left != NULL)
                        sibling->left->color = BLACK;
                    sibling->color = RED;
                    rotate_right(root, sibling);
                    sibling = parent_x->right;
                }
                sibling->color = parent_x->color;
                parent_x->color = BLACK;
                if (sibling->right != NULL)
                    sibling->right->color = BLACK;
                rotate_left(root, parent_x);
                //x = *root;
                break;
            }
        } else {
            rb_node *sibling = parent_x->left;
            if (sibling->color == RED) {
                sibling->color = BLACK;
                parent_x->color = RED;
                rotate_right(root, parent_x);
                sibling = parent_x->left;
            }
            if ((sibling->right == NULL || sibling->right->color == BLACK) &&
                (sibling->left == NULL || sibling->left->color == BLACK)) {
                sibling->color = RED;
                x = parent_x;
                if (x->parent != NULL)
                    is_left = (x == x->parent->left);
                parent_x = x->parent;
            } else {
                if (sibling->left == NULL || sibling->left->color == BLACK) {
                    if (sibling->right != NULL)
                        sibling->right->color = BLACK;
                    sibling->color = RED;
                    rotate_left(root, sibling);
                    sibling = parent_x->left;
                }
                
                sibling->color = parent_x->color;
                parent_x->color = BLACK;
                if (sibling->left != NULL)
                    sibling->left->color = BLACK;
                rotate_right(root, parent_x);
                //x = *root;
                break;
            }
        }
    }
    
    if (x != NULL)
        x->color = BLACK;
}

void deletion(rbtree *tree, CustomData* data) {
    if(tree->count <= 0 || tree->root == NULL) return;
    delete_node(&(tree->root),data);
    tree->count--;
}
// Delete a rb_node
void delete_node(rb_node **root, CustomData* data) {
    //if (*root == NULL) return;
    
    rb_node *z = find_node(*root, data);
    if (z == NULL) return; 
    
    rb_node *y = z;
    rb_node *x = NULL;
    rb_node *parent_x = NULL;
    rb_color original_color = y->color;
    bool is_left = false;
    
    if (z->left == NULL) {
        x = z->right;
        parent_x = z->parent;
        is_left = (z->parent != NULL && z == z->parent->left);
        if (x != NULL)
            x->parent = z->parent;
        
        if (z->parent == NULL)
            *root = x;
        else if (z == z->parent->left)
            z->parent->left = x;
        else
            z->parent->right = x;
    } 
    else if (z->right == NULL) {
        x = z->left;
        parent_x = z->parent;
        is_left = (z->parent != NULL && z == z->parent->left);

        x->parent = z->parent;
        
        if (z->parent == NULL)
            *root = x;
        else if (z == z->parent->left)
            z->parent->left = x;
        else
            z->parent->right = x;
    }
    else {
        y = min_right_tree(z);
        original_color = y->color;
        x = y->right;
        
        if (y->parent == z) {
            parent_x = y;
            is_left = false;         
            //if (x != NULL)
                //x->parent = y;
        } 
        else {
            parent_x = y->parent;
            is_left = (y == y->parent->left);

            if (x != NULL)
                x->parent = y->parent;
            
            y->parent->left = x;
            
            y->right = z->right;
            z->right->parent = y;
        }
        
        if (z->parent == NULL)
            *root = y;
        else if (z == z->parent->left)
            z->parent->left = y;
        else
            z->parent->right = y;
        
        y->parent = z->parent;
        y->left = z->left;
        z->left->parent = y;
        y->color = z->color;
    }

    if (original_color == BLACK && parent_x != NULL)
        fix_delete(root, x, parent_x, is_left);
    
    free(z);
}

/*void in_order_traversal(rb_node *root) {
    if (root != NULL) {
        in_order_traversal(root->left);
        printf("%d(%d-%c) ", root->data->key, root->data->timestamp, (root->color == RED) ? 'R' : 'B');
        in_order_traversal(root->right);
    }
}


rb_queue* create_queue() {
    rb_queue *queue = (rb_queue*)malloc(sizeof(rb_queue));
    if (queue == NULL) {
        exit(1);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

void enqueue(rb_queue *queue, rb_node *data, int level) {
    rb_queue_node *newNode = (rb_queue_node*)malloc(sizeof(rb_queue_node));
    if (newNode == NULL) {
        exit(1);
    }
    
    newNode->data = data;
    newNode->level = level;
    newNode->next = NULL;
    
    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
        return;
    }
    
    queue->rear->next = newNode;
    queue->rear = newNode;
}

rb_queue_node* dequeue(rb_queue *queue) {
    if (queue->front == NULL)
        return NULL;
    
    rb_queue_node *temp = queue->front;
    queue->front = queue->front->next;
    
    if (queue->front == NULL)
        queue->rear = NULL;
    
    return temp;
}

bool is_queue_empty(rb_queue *queue) {
    return queue->front == NULL;
}

void level_order_traversal(rb_node *root) {
    if (root == NULL) {
        printf("Tree is empty\n");
        return;
    }
    
    rb_queue *queue = create_queue();
    enqueue(queue, root, 0);
    
    int current_level = 0;
    printf("Level %d: ", current_level);
    
    while (!is_queue_empty(queue)) {
        rb_queue_node *qNode = dequeue(queue);
        rb_node *current = qNode->data;
        int node_level = qNode->level;
        
        if (node_level > current_level) {
            current_level = node_level;
            printf("\nLevel %d: ", current_level);
        }

        printf("%d(%d-%c) ", current->data->key, current->data->timestamp, (current->color == RED) ? 'R' : 'B');
        
        if (current->left != NULL)
            enqueue(queue, current->left, node_level + 1);
        
        if (current->right != NULL)
            enqueue(queue, current->right, node_level + 1);
        
        free(qNode);
    }
    
    printf("\n");
    free(queue);
}*/
void free_tree(rbtree *tree){
    free_node(tree->root);
}
void free_node(rb_node *root) {
    if (root != NULL) {
        free_node(root->left);
        free_node(root->right);
        free(root);
    }
}
CustomData* createData(struct pcb_t *proc, int timestamp){
    CustomData *data = (CustomData*)malloc(sizeof(CustomData));
    data->proc = proc;
    data->key = proc->vruntime;
    data->timestamp = timestamp;
    return data;
}
void traverse(rb_node *root, void (*visit)(rb_node *node), Traversal order) {
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