#include "thread-worker.h"
#include <stdbool.h>

typedef struct node {
    tcb* t_block;
    struct node* next; 
} node;

int queue_add(node** head, tcb* block) {

    if (*head == NULL) {
       node* new_node = malloc(sizeof(node));
        new_node->t_block = block;
        new_node->next = NULL;

        *head = new_node; 
    }
    else{
        node* temp = *head;
        while (temp->next != NULL) {  
            temp = temp->next;
        }
        node* new_node = malloc(sizeof(node));
        new_node->t_block = block;
        new_node->next = NULL;

        temp->next = new_node;
    }
    return 0;
    
}

node* queue_front(node** head){
    if (*head == NULL) return NULL;
    return *head;
}

void queue_pop(node** head){
    if (*head == NULL) return;
    *head = (*head)->next;
}

bool is_empty(node** head){
    return (*head == NULL);
}