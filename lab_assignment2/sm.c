/**
 * CS2106 AY 20/21 Semester 1 - Lab 2
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#include "sm.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

pid_t pids[32]; //global array to store the child pids.
bool bools[32]; //global array to store the running states.
size_t x = 0;
char* msg;


int w1,wstatus;


// Use this function to any initialisation if you need to.
void sm_init(void) {
}

// Use this function to do any cleanup of resources.
void sm_free(void) {
}

// Exercise 1a/2: start services
void sm_start(const char *processes[]) {
	printf("%s\n", processes[0]);
	msg = malloc(strlen(processes[0]) + 1);
	strcpy(msg, processes[0]);
	//paths[x] = processes[0];
	pid_t ret = fork(); //returns the process ID 
	//of the child process to the parent process
	if (ret == 0) {
		//child process
		execv(processes[0], processes);
	}
	else {
		pids[x] = ret;
		x++;
		//parent process
	}
}

// Exercise 1b: print service status
size_t sm_status(sm_status_t statuses[]) {
	//siginfo_t siginfo;
	
	/*printf("%d\n", pids[0]);
	printf("%d\n", pids[8]);*/

	size_t i = 0;
	while (pids[i] != 0) {
		//w1 = wait(&wstatus);
		sm_status_t* status = statuses + i;
		status->path = msg;
		status->pid = pids[i];
		waitpid(pids[i], &w1, WNOHANG);
		status->running = (w1 == 0);
		i++;
	}	
		/*wait();*/ // to make it not wait, want it to spit out current status
	return x; // the number of valid pids in your global array.
}

// Exercise 3: stop service, wait on service, and shutdown
void sm_stop(size_t index) {
}

void sm_wait(size_t index) {
}

void sm_shutdown(void) {
}

// Exercise 4: start with output redirection
void sm_startlog(const char *processes[]) {
}

// Exercise 5: show log file
void sm_showlog(size_t index) {
}
