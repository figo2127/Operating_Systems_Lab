/*************************************
* Lab 1 Exercise 1
* Name: Lee Ze Xin
* Student No: A0203252X
* Lab Group: LAB[14]
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "node.h"

// add in your implementation below to the respective functions
// feel free to add any headers you deem fit (although you do not need to)

int getListLength(list* lst) {
    if (lst->head == NULL && lst->tail == NULL) {
        return 0;
    }
    else {
        int count = 1;
        node* head_tracker = NULL;
        head_tracker = lst->head;
        while (head_tracker->next != NULL) {
            head_tracker = head_tracker->next;
            count++;
        }
        return count;
    }
}

// inserts a new node with data value at index (counting from the front
// starting at 0)
void insert_node_from_head_at(list* lst, int index, int data)
{
    if (lst->head == NULL && lst->tail == NULL) {
        // if lst provided is empty 

        node* newNode = (node*)malloc(sizeof(node));
        newNode->data = data;
        newNode->next = NULL;
        newNode->prev = NULL;
        lst->head = newNode;
        lst->tail = newNode;
    }
    else {
        // if inserting at the first position
        if (index == 0) {
            node* newNode = (node*)malloc(sizeof(node));
            newNode->data = data;
            newNode->next = lst->head;
            lst->head->prev = newNode;
            newNode->prev = NULL;
            lst->head = newNode;
        }
        // if inserting in the back of the linked list
        else if (index == getListLength(lst)) {
            node* ptr = lst->tail;
            /*while (ptr->next != NULL) {
                ptr = ptr->next;
            }*/
            node* newNode = (node*)malloc(sizeof(node));
            ptr->next = newNode;
            newNode->data = data;
            newNode->prev = ptr;
            newNode->next = NULL;
            lst->tail = newNode;
        }
        // if inserting in the middle
        else {
            node* ptr = lst->head;
            while ((index - 1) != 0) {
                ptr = ptr->next;
                index--;
            }
            node* storeNext = ptr->next;
            node* newNode = (node*)malloc(sizeof(node));
            newNode->data = data;
            ptr->next = newNode;
            newNode->prev = ptr;
            newNode->next = storeNext;
            storeNext->prev = newNode;
        }
    }

}

// inserts a new node with data value at index (counting from the back
// starting at 0)
void insert_node_from_tail_at(list* lst, int index, int data)
{
    if (lst->head == NULL && lst->tail == NULL) {
        node* newNode = (node*)malloc(sizeof(node));
        newNode->data = data;
        newNode->next = NULL;
        newNode->prev = NULL;
        lst->head = newNode;
        lst->tail = newNode;
    }
    else {
        // if inserting at the last position
        if (index == 0) {
            node* ptr = lst->tail;
            /*while (ptr->next != NULL) {
                ptr = ptr->next;
            }*/
            node* newNode = (node*)malloc(sizeof(node));
            newNode->data = data;
            ptr->next = newNode;
            newNode->prev = ptr;
            newNode->next = NULL;
            lst->tail = newNode;
        }
        // if inserting in the front of the linked list
        else if (index == getListLength(lst)) {
            node* newNode = (node*)malloc(sizeof(node));
            newNode->data = data;
            newNode->next = lst->head;
            lst->head->prev = newNode;
            newNode->prev = NULL;
            lst->head = newNode;
        }
        // if inserting in the middle
        else {
            node* ptr = lst->tail;
            while ((index - 1) != 0) {
                ptr = ptr->prev;
                index--;
            }
            node* newNode = (node*)malloc(sizeof(node));
            newNode->data = data;
            node* storeNext = ptr->prev;
            ptr->prev = newNode;
            newNode->next = ptr;
            newNode->prev = storeNext;
            storeNext->next = newNode;
        }
    }
}

// deletes node at index counting from the front (starting from 0)
// note: index is guaranteed to be valid
void delete_node_from_head_at(list* lst, int index)
{
    if ((lst->head == NULL && lst->tail == NULL) || index >= getListLength(lst)) {
        // if lst provided is empty, do nothing
    }
    else {
        // if deleting at first position
        if (index == 0) {
            node* ptr = lst->head;
            lst->head = lst->head->next;
            lst->head->prev = NULL;
            free(ptr);
        }
        // if deleting at last position
        else if (index == getListLength(lst) - 1) {
            node* ptr = lst->head;
            while (ptr->next != NULL) {
                ptr = ptr->next;
            }
            node* store_prev = ptr->prev;
            store_prev->next = NULL;
            lst->tail = store_prev;
            free(ptr);
        }
        else {
            node* ptr = lst->head;
            while (index != 0) {
                ptr = ptr->next;
                index--;
            }
            node* store_next = ptr->next;
            node* store_prev = ptr->prev;
            store_next->prev = store_prev;
            store_prev->next = store_next;
            free(ptr);
        }
    }
}

// deletes node at index counting from the back (starting from 0)
// note: index is guaranteed to be valid
void delete_node_from_tail_at(list* lst, int index)
{
    if ((lst->head == NULL && lst->tail == NULL) || index >= getListLength(lst)) {
        // if lst provided is empty, do nothing
    }
    else {
        // if deleting at first position
        if (index == getListLength(lst) - 1) {
            node* ptr = lst->head;
            lst->head = lst->head->next;
            lst->head->prev = NULL;
            free(ptr);
        }
        // if deleting at last position
        else if (index == 0) {
            node* ptr = lst->tail;
            node* store_prev = ptr->prev;
            store_prev->next = NULL;
            lst->tail = store_prev;
            free(ptr);
        }
        else {
            node* ptr = lst->tail;
            while (index != 0) {
                ptr = ptr->prev;
                index--;
            }
            node* store_next = ptr->next;
            node* store_prev = ptr->prev;
            store_next->prev = store_prev;
            store_prev->next = store_next;
            free(ptr);
        }
    }
}

// resets list to an empty state (no nodes) and frees any allocated memory in
// the process
void reset_list(list *lst)
{   
    node* ptr = lst->head;
    while (ptr != NULL) {
        node* temp = ptr;
        ptr = ptr->next;
        free(temp);
    }
    lst->head = NULL;
    lst->tail = NULL;
}
