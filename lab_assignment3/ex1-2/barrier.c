/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#include "barrier.h"

 // Initialise barrier here
void barrier_init(barrier_t* barrier, int count) {
    barrier->count = count;
    barrier->n = count;

    barrier->mutex = malloc(sizeof(sem_t));
    sem_init(barrier->mutex, 0, 1); //init mutex to value 1 

    barrier->reach_semaphore = malloc(sizeof(sem_t));
    sem_init(barrier->reach_semaphore, 0, 0);

    barrier->join_semaphore = malloc(sizeof(sem_t));
    sem_init(barrier->join_semaphore, 0, 0);
}

void wait_barr_one(barrier_t* barrier) {
    sem_wait(barrier->mutex); //If the value of the semaphore is negative,
                              //the calling process blocks;
                              //one of the blocked processes wakes up when another process calls sem_post.
    //start of CS
    barrier->count = barrier->count-1; //decrement count each time as each thread arrives to the barrier.
    if (barrier->count == 0) { //only enters this region when all threads have arrived at the barrier.
        for (int i = 0; i < barrier->n; i++)
            sem_post(barrier->reach_semaphore); //Flushes everyone waiting through the barrier.
    }
    //end of CS
    sem_post(barrier->mutex);

    sem_wait(barrier->reach_semaphore); //threads wait here if barrier is not full yet.
}

void wait_barr_two(barrier_t* barrier) {
    sem_wait(barrier->mutex);

    //start of CS
    //int x = barrier->count;
    barrier->count = barrier->count + 1;
    if (barrier->count == barrier->n) {
        for (int i = 0; i < barrier->n; i++)
            sem_post(barrier->join_semaphore);
    }
    //end of CS
    sem_post(barrier->mutex);

    sem_wait(barrier->join_semaphore);
}

void barrier_wait(barrier_t* barrier) {
    wait_barr_one(barrier);
    wait_barr_two(barrier);
}



// Perform cleanup here if you need to
void barrier_destroy(barrier_t* barrier) {
    /*sem_destroy(barrier->mutex);
    free(barrier->mutex);

    sem_destroy(barrier->reach_semaphore);
    free(barrier->reach_semaphore);

    sem_destroy(barrier->join_semaphore);
    free(barrier->join_semaphore);*/

    sem_destroy(barrier->mutex);
    if (barrier->mutex != NULL)
        free(barrier->mutex); //free the dynamically allocateed memory
    barrier->mutex = NULL;

    sem_destroy(barrier->reach_semaphore);
    if (barrier->reach_semaphore != NULL)
        free(barrier->reach_semaphore);
    barrier->reach_semaphore = NULL;

    sem_destroy(barrier->join_semaphore);
    if (barrier->join_semaphore != NULL)
        free(barrier->join_semaphore);
    barrier->join_semaphore = NULL;
}


// BUSY WAITING VER:
//// Initialise barrier here
//void barrier_init ( barrier_t *barrier, int count ) {
//    barrier->count = count; //assign thread count 
//
//}
//
//void barrier_wait ( barrier_t *barrier ) {
//    int curr = barrier->num;
//    curr++;
//    barrier->num = curr;
//    while (barrier->num != barrier->count) {
//        //busy waiting
//    }
//}
//
//// Perform cleanup here if you need to
//void barrier_destroy ( barrier_t *barrier ) {
//    barrier->num = 0;
//    barrier->count = 0;
//}
