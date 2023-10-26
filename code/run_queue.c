#include "thread-worker.h"
#include <stdbool.h>

typedef struct node {
    tcb* t_block;
    struct node* next; 
} node;


//adds node to back of queue
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

void remove_node(node** head, node* node_to_remove) {
    if (*head == NULL || node_to_remove == NULL) {
        // List is empty or the node to remove is NULL, nothing to do
        return;
    }

    if (*head == node_to_remove) {
        // The node to remove is the head of the list
        *head = (*head)->next;
        return;
    }

    node* current = *head;
    while (current != NULL && current->next != node_to_remove) {
        current = current->next;
    }

    if (current != NULL) {
        current->next = node_to_remove->next;
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

// Function to move every node from h2 to the end of h1, sets the priorities of nodes in h2 to h1 priority
void queue_moveNodes(node** h1, node** h2, int h1_priority) {
    if (*h2 == NULL) {
        return;  // Nothing to move from h2
    }

    //change priority of h2 nodes to h1
    node* current = *h2;
    while (current != NULL) {
        current->t_block->priority = h1_priority;
        current = current->next;
    }

    // If h1 is empty, update its head to point to the nodes in h2
    if (*h1 == NULL) {
        *h1 = *h2;
    } else {
        // Find the last node in h1
        node* lastNode = *h1;
        while (lastNode->next != NULL) {
            lastNode = lastNode->next;
        }
        
        // Connect the last node in h1 to the head of h2
        lastNode->next = *h2;
    }

    // Clear h2 by setting its head to NULL
    *h2 = NULL;
}

// Function to find the TCB with the lowest quantum count and move it to the front of the runqueue
void move_lowest_quantum_to_front(node** head) {
    if (is_empty(head)) {
        // Runqueue is empty, nothing to do
        return;
    }

    node* min_node = *head;
    node* current = (*head)->next;

    // Find the node with the lowest quantum count and status "READY"
    while (current != NULL) {
        if (current->t_block->status == READY){
            if (min_node->t_block->status != READY){
                min_node = current;
            } else if (current->t_block->quantum_counter < min_node->t_block->quantum_counter){
                    min_node = current;
            }
        }
        current = current->next;
    }

    // Remove the node with the lowest quantum count from its current position
    remove_node(head, min_node);

    // Add the node with the lowest quantum count to the front of the runqueue
    add_front(head, min_node);
}

/*prints queue size*/
int queue_size(node** head) {
    int size = 0;
    node* current = *head;

    while (current != NULL) {
        size++;
        current = current->next;
    }

    return size;
}



