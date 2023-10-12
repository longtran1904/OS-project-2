#include "thread-worker.h"

typedef struct {
    tcb t_block;
    node* next; 
} node;

static void queue_add(node* head, tcb block) {
    while (head->next != NULL) {  
        head = head->next;
    }

    node* new_node;
    new_node->t_block = block;
    new_node->next = NULL;

    head->next = new_node;
}

static node* queue_pop(node* head){
    node* pop = head;
    head = head->next;
    return pop;
}