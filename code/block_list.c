#include "thread-worker.h"
#include <stdbool.h>

typedef struct block_node {
    worker_mutex_t* mutex;
    struct node* current_thread;
    struct block_node* next; 
} block_node;

int list_add(block_node** head, block_node* new_block_node) {

    if (*head == NULL) {
        *head = new_block_node; 
    }
    else{
        block_node* temp = *head;
        while (temp->next != NULL) {  
            temp = temp->next;
        }

        temp->next = new_block_node;
    }
    return 0;
    
}

block_node* list_find(block_node** head, worker_mutex_t* mutex){
    block_node* tmp = *head;
    block_node* prev = NULL;
    while (tmp != NULL){
        if (tmp->mutex == mutex)
        {
            if (prev == NULL) 
            {   
                // node found at begin list
                *head = (*head)->next; 
            }
            else if (tmp->next == NULL) {
                // node found at end list
                prev->next = NULL;
            }
            else {
                prev->next = tmp->next;
            }
            return tmp;
        }   
        prev = tmp;
        tmp = tmp->next;
    }
    return NULL;
}

bool list_empty(block_node** head){
    return *head == NULL;
}