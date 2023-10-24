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

// Function to find the TCB with the lowest quantum count and move it to the front of the runqueue
void move_lowest_quantum_to_front(node** head) {
    if (is_empty(*head)) {
        // Runqueue is empty, nothing to do
        return;
    }

    node* min_node = *head;
    node* current = (*head)->next;

    // Find the node with the lowest quantum count and status "READY"
    while (current != NULL) {
        if (current->t_block->quantum_counter < min_node->t_block->quantum_counter &&
            current->t_block->status == READY) {
            min_node = current;
        }
        current = current->next;
    }

    // Remove the node with the lowest quantum count from its current position
    remove_node(head, min_node);

    // Add the node with the lowest quantum count to the front of the runqueue
    add_front(head, min_node);
}

void remove_node(node** head, node* node_to_remove) {
    if (*head == NULL || node_to_remove == NULL) {
        // List is empty or the node to remove is NULL, nothing to do
        return;
    }

    if (*head == node_to_remove) {
        // The node to remove is the head of the list
        *head = (*head)->next;
        free(node_to_remove);
        return;
    }

    node* current = *head;
    while (current != NULL && current->next != node_to_remove) {
        current = current->next;
    }

    if (current != NULL) {
        current->next = node_to_remove->next;
        free(node_to_remove);
    }
}

// Add a node to the front of the linked list
void add_front(node** head, node* new_node) {
    if (new_node == NULL) {
        return;
    }

    new_node->next = *head;
    *head = new_node;
}



