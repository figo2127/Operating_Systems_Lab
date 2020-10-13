/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * This file contains declarations. You should only modify the fifo_sem_t struct,
 * as the method signatures will be needed to compile with the runner.
 */
#include "exit_controller.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void exit_controller_init(exit_controller_t* exit_controller, int no_of_priorities) {
    exit_controller->exitQueue = (list*)malloc(sizeof(list));
    exit_controller->exitQueue->head = NULL;
    exit_controller->exitQueue->tail = NULL;
    sem_init(&(exit_controller->mutex), 0, 1);
    sem_init(&(exit_controller->exitSem), 0, 1);
}

void exit_controller_wait(exit_controller_t* exit_controller, int priority) {
    sem_wait(&(exit_controller->mutex));
    //start CS
    sem_t currTrain;
    sem_init(&currTrain, 0, 1);
    //since we have only 2 priorities, we can insert those with higher priority
    //to the front of the exitQueue and those with lower priority to the back
    //of the exitQueue.
    if (priority == 0) {
        insertTrainToFront(exit_controller->exitQueue, currTrain);
    }
    if (priority == 1) {
        insertTrainToBack(exit_controller->exitQueue, currTrain);
    }
    sem_wait(&currTrain);
    //end CS
    sem_post(&(exit_controller->mutex));

    sem_wait(&(exit_controller->exitSem)); //needs exit_controller_post to signal in order to get pass to Exit
    sem_wait(&(exit_controller->mutex));
    currTrain = *(retrieveFromHead(exit_controller->exitQueue));
    sem_post(&currTrain);
    sem_post(&exit_controller->mutex);

}

void exit_controller_post(exit_controller_t* exit_controller, int priority) {
    sem_post(&(exit_controller->exitSem));
}

void exit_controller_destroy(exit_controller_t* exit_controller) {
    sem_destroy(&(exit_controller->exitSem));
    sem_destroy(&(exit_controller->mutex));
    reset_list(exit_controller->exitQueue);

}

//HELPERS
void insertTrainToFront(list* lst, sem_t data) //for priority == 0
{
    //if list is empty
    if (lst->head == NULL && lst->tail == NULL)
    {
        node* ptr = (node*)malloc(sizeof(node));
        lst->head = ptr;
        lst->tail = ptr;
        ptr->data = data;
        ptr->next = NULL;
        ptr->prev = NULL;
    }
    else //if list is not empty
    {
        node* currNode = lst->head;
        //node* prevNode = NULL;
        node* newNode = (node*)malloc(sizeof(node));

        newNode->data = data;
        newNode->next = currNode;
        newNode->prev = currNode->prev;
        currNode->prev = newNode;
        lst->head = newNode;
    }
}

// inserts a new node with data value at index (counting from the back
// starting at 0)
void insertTrainToBack(list* lst, sem_t data) //for priority == 1
{
    //if list is empty
    if (lst->head == NULL && lst->tail == NULL)
    {
        node* ptr = (node*)malloc(sizeof(node));
        lst->head = ptr;
        lst->tail = ptr;
        ptr->data = data;
        ptr->next = NULL;
        ptr->prev = NULL;
    }
    else
    {
        node* currNode = lst->tail;
        //node* prevNode = NULL;
        node* newNode = (node*)malloc(sizeof(node));

        newNode->data = data; //assign data
        newNode->next = currNode->next; 
        newNode->prev = currNode;
        currNode->next = newNode; //connect the next of currNode to newNode
        lst->tail = newNode;
     
    }
}

// deletes node at index counting from the front (starting from 0)
// note: index is guaranteed to be valid
sem_t* retrieveFromHead(list* lst)
{
    node* currNode = lst->head;
    node* outputNode = lst->head;
    /* base case*/
    if (lst->head == NULL || currNode == NULL) {
        return &(outputNode->data);
    }
    //retrieving the last node at head
    if (currNode->prev == NULL && currNode->next == NULL) {
        node* ptr = lst->head;
        free(ptr);
        lst->head = NULL;
        lst->tail = NULL;
    }
    else {
        lst->head = currNode->next; //set head to the next node
        lst->head->prev = NULL; //new head should have nothing in front of it
        //free the current node
         currNode->prev = NULL;
         currNode->next = NULL;
         free(currNode);
    }
    return &(outputNode->data);
}

// resets list to an empty state (no nodes) and frees any allocated memory in
// the process
void reset_list(list* lst)
{
    node* ptr = lst->head;
    while (ptr != NULL)
    {
        node* next = ptr->next;
        free(ptr);
        ptr = next;
    }
    lst->head = NULL;
    lst->tail = NULL;
}