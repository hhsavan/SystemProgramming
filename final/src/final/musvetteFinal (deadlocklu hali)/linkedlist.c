#include "linkedlist.h"
#include <stdlib.h>
#include <stdio.h>

ListNode* newListNode(void* data) {
    ListNode* node = (ListNode*)malloc(sizeof(ListNode));
    node->data = data;
    node->next = NULL;
    return node;
}

LinkedList* createLinkedList() {
    LinkedList* list = (LinkedList*)malloc(sizeof(LinkedList));
    list->head = NULL;
    pthread_mutex_init(&list->mutex, NULL);
    return list;
}

void insertAtBeginning(LinkedList* list, void* data) {
    pthread_mutex_lock(&list->mutex);
    ListNode* node = newListNode(data);
    node->next = list->head;
    list->head = node;
    pthread_mutex_unlock(&list->mutex);
}

void insertAtEnd(LinkedList* list, void* data) {
    pthread_mutex_lock(&list->mutex);
    ListNode* node = newListNode(data);
    if (list->head == NULL) {
        list->head = node;
    } else {
        ListNode* temp = list->head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = node;
    }
    pthread_mutex_unlock(&list->mutex);
}

bool deleteNode(LinkedList* list, void* data) {
    pthread_mutex_lock(&list->mutex);
    ListNode* temp = list->head;
    ListNode* prev = NULL;
    while (temp != NULL && temp->data != data) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) {
        pthread_mutex_unlock(&list->mutex);
        return false;
    }
    if (prev == NULL) {
        list->head = temp->next;
    } else {
        prev->next = temp->next;
    }
    free(temp);
    pthread_mutex_unlock(&list->mutex);
    return true;
}

bool search(LinkedList* list, void* data) {
    pthread_mutex_lock(&list->mutex);
    ListNode* temp = list->head;
    while (temp != NULL) {
        if (temp->data == data) {
            pthread_mutex_unlock(&list->mutex);
            return true;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&list->mutex);
    return false;
}

void printList(LinkedList* list) {
    pthread_mutex_lock(&list->mutex);
    ListNode* temp = list->head;
    while (temp != NULL) {
        printf("%p -> ", temp->data);
        temp = temp->next;
    }
    printf("NULL\n");
    pthread_mutex_unlock(&list->mutex);
}

void freeLinkedList(LinkedList* list) {
    pthread_mutex_lock(&list->mutex);
    ListNode* temp = list->head;
    while (temp != NULL) {
        ListNode* next = temp->next;
        free(temp);
        temp = next;
    }
    pthread_mutex_unlock(&list->mutex);
    pthread_mutex_destroy(&list->mutex);
    free(list);
}
