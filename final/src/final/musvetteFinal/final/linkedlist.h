#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdbool.h>
#include <pthread.h>

// Structure for a linked list node
typedef struct ListNode {
    void* data;
    struct ListNode* next;
} ListNode;

// Structure for a linked list
typedef struct LinkedList {
    ListNode* head;
    pthread_mutex_t mutex;
} LinkedList;

// Function to create a new list node
ListNode* newListNode(void* data);

// Function to create an empty linked list
LinkedList* createLinkedList();

// Function to add an item at the beginning
void insertAtBeginning(LinkedList* list, void* data);

// Function to add an item at the end
void insertAtEnd(LinkedList* list, void* data);

// Function to remove an item from the list
bool deleteNode(LinkedList* list, void* data);

// Function to search for an item in the list
bool search(LinkedList* list, void* data);

// Function to print the list (for debugging)
void printList(LinkedList* list);

// Function to free the linked list
void freeLinkedList(LinkedList* list);

#endif
