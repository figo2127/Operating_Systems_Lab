/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * This file contains declarations. You should only modify the barrier_t struct,
 * as the method signatures will be needed to compile with the runner.
 */
#include <stdlib.h>
#include <semaphore.h>
#ifndef __CS2106_BARRIER_H_
#define __CS2106_BARRIER_H_

typedef struct barrier {
	int count;
	// add additional fields here
	int n; //number of threads
	sem_t *mutex; 
	sem_t *reach_semaphore;
	sem_t *join_semaphore;
} barrier_t;

void barrier_init ( barrier_t *barrier, int count );
void barrier_wait ( barrier_t *barrier );
void barrier_destroy ( barrier_t *barrier );

#endif // __CS2106_BARRIER_H_
