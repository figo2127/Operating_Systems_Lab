/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * This file contains declarations. You should only modify the exit_controller_t struct,
 * as the method signatures will be needed to compile with the runner.
 */
#ifndef __CS2106_EXIT_CONTROLLER_H_
#define __CS2106_EXIT_CONTROLLER_H_

#include <semaphore.h>
#include <pthread.h>

#define MAX_PRIORITIES 5000 // we set a limit on how many possible priorities we have

//taken from lab1
typedef struct NODE
{
	struct NODE* prev;
	struct NODE* next;
	sem_t data;
} node;

typedef struct
{
	node* head;
	node* tail;
} list;

typedef struct exit_controller {
	// define your variables here
	sem_t mutex;
	sem_t exitSem;
	list* exitQueue;
} exit_controller_t;

void exit_controller_init(exit_controller_t* exit_controller, int no_of_priorities);
void exit_controller_wait(exit_controller_t* exit_controller, int priority);
void exit_controller_post(exit_controller_t* exit_controller, int priority);
void exit_controller_destroy(exit_controller_t* exit_controller);

//helper functions to aid with exitQueue implementation
void insertTrainToFront(list* lst, sem_t data);
void insertTrainToBack(list* lst, sem_t data);
sem_t* retrieveFromHead(list* lst);
void reset_list(list* lst);


#endif // __CS2106_EXIT_CONTROLLER_H_
