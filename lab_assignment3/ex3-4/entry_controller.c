/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * Your implementation should go in this file.
 */
#include "entry_controller.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//pthread method:
//int pthread_cond_wait(pthread_cond_t* restrict cond,
//    pthread_mutex_t* restrict mutex);
//broadcast and only wake the thread who is matches the cond, others put back to sleep

void entry_controller_init(entry_controller_t* entry_controller, int loading_bays) {
    entry_controller->front = 0;
    entry_controller->back = -1;
    entry_controller->trainCount = 0;
    sem_init(&(entry_controller->loadingBay), 0, loading_bays);  //num of loading bays assigned to loadingBay semaphore
    sem_init(&(entry_controller->mutex), 0, 1);
}

void entry_controller_wait(entry_controller_t* entry_controller) {
    //create currTrain semaphore

    //each train is a semaphore on its own

    // "joins the loading bay queue"
    sem_wait(&(entry_controller->mutex));
    //start CS
    sem_t currTrain; //no need for malloc -Cristina
    sem_init(&currTrain, 0, 1); //init to 1
    //insert the current train into the array waiting to go into loadingBay
    addToWaitingTrains(currTrain, entry_controller);
    //end CS
    sem_post(&(entry_controller->mutex));

    sem_wait(&(entry_controller->loadingBay)); //wait for loadingBay, if loadingBay is vacant will go in
                                        //only gets pass when entry_controller_post function is called
                                        
    sem_wait(&currTrain);
    sem_wait(&(entry_controller->mutex));
    currTrain = pollTrain(entry_controller); //prepares to enter loading loadingBay
    sem_post(&currTrain);
    sem_post(&(entry_controller->mutex));
}

void entry_controller_post(entry_controller_t* entry_controller) {
    sem_post(&(entry_controller->loadingBay));
}

void entry_controller_destroy(entry_controller_t* entry_controller) {
    //destroy all semaphores
    sem_destroy(&(entry_controller->loadingBay));
    sem_destroy(&(entry_controller->mutex));
}

//Helpers for our entering train mutex
void addToWaitingTrains(sem_t SEM, entry_controller_t* entry_controller) {
    if (entry_controller->trainCount != ENTRY_CONTROLLER_MAX_USES) {
        if (entry_controller->back == ENTRY_CONTROLLER_MAX_USES - 1) {
            entry_controller->back = -1;
        }
        //will increment back by 1 BEFORE assigning into array
        entry_controller->trainsOfSemsARR[++entry_controller->back] = SEM;
        entry_controller->trainCount++;
    }
}

sem_t pollTrain(entry_controller_t* entry_controller) { //poll train(semaphore) to put into loading loadingBay
    //increment of front by 1 will take effect AFTERWARDS.
    sem_t train = entry_controller->trainsOfSemsARR[entry_controller->front++];
    if (entry_controller->front == ENTRY_CONTROLLER_MAX_USES) {
        entry_controller->front = 0;
    }
    entry_controller->trainCount--;
    return train;
}





