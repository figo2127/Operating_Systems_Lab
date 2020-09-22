/**
 * CS2106 AY 20/21 Semester 1 - Lab 2
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#include "sm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/wait.h>
#define SM_MAX_PROCESSES 32


char* my_paths[SM_MAX_SERVICES];
//declare 2D array to store PIDs.
int my_PIDs[SM_MAX_SERVICES][SM_MAX_PROCESSES];
//outer layer is a service, a service can have many processes
char* token;
char* log_path;
sm_status_t* status;
int process_num;

// Use this function to any initialisation if you need to.
void sm_init(void) {
    status = (sm_status_t*)malloc(sizeof(sm_status_t));
    int s = sizeof(char*);
    log_path = (char*)malloc(s);
    token = (char*)malloc(s);
    process_num = 0;
}

// Use this function to do any cleanup of resources.
void sm_free(void) {
    int n = 0;
    while (n < process_num) {
        free(my_paths[n]);
        n++;
    }
    free(log_path);
    free(status);
    free(token);
}

// Exercise 1a/2: start services
void sm_start(const char* processes[]) {
    char* allCommands[SM_MAX_SERVICES][SM_MAX_PROCESSES];
    int x, y, a, b;
    a = 0;
    b = 0;
    bool end = false;
    //shove the processess into our 2D array for ease of processing.
    while (!end) {
        //check for the end of processes, which is 2 consecutive NULLs.
        if (processes[b] == NULL) {
            end = true;
            break;
        }
        x = 0; //reset x
        while (processes[b] != NULL) {
            allCommands[a][x] = (char*)malloc(sizeof(char*));
            strcpy(allCommands[a][x], processes[b]);
            b++;
            x++;
        }
        allCommands[a][x] = NULL;
        b++;
        a++;
        //jump onto next inner array.
    }

    char* path = (char*)malloc(sizeof(char*));
    int childPID;
    int fd[a][2]; // collection of file descriptors

    char* temp;

    for (y = 0; y < a; y++) {
        strcpy(path, allCommands[y][0]); //copy the path of process into 'path'
        temp = strtok(path, "/"); //split up
        while (temp != NULL) {
            strcpy(token, temp); //copy p into token
            temp = strtok(NULL, "/"); //continue split with the same string 
        }
        strcpy(path, allCommands[y][0]);
        free(allCommands[y][0]);
        allCommands[y][0] = token;
        pipe(fd[y]);
        childPID = fork();
        
        if (childPID == 0) {
            //this is the child process
            if (y != 0) {
                //there's previous commands
                //need to read from pipe
                //if (dup2(fd[y - 1][0], 0) == -1) {
                //    /* Handle dup2() error */
                //    
                //}
                dup2(fd[y - 1][0], 0); //connect read end of pipe to stdin
                close(fd[y - 1][0]);
                close(fd[y - 1][1]);
                
            }

            if (y != a - 1) {
                //not last process there's upcoming commands
                //have to write to pipe
                dup2(fd[y][1], 1); //connect write end of pipe to stdout
                close(fd[y][0]);
                close(fd[y][1]);
                
            }
            execv(path, allCommands[y]);
            
        }
        else {
            //parent process
            my_PIDs[process_num][y + 1] = childPID;
            if (y != 0) {
                close(fd[y - 1][0]);
                close(fd[y - 1][1]);
            }
            waitpid(childPID, NULL, WNOHANG);
        }
    }
    my_PIDs[process_num][0] = childPID;
    my_PIDs[process_num][a] = -1;
    my_paths[process_num] = path;
    process_num++;

    //close all the file descriptors, we're done
    for (x = 0; x < a; x++) {
        close(fd[x][0]);
        close(fd[x][1]);
    }
    
    for (y = 0; y < a; y++) {
        b = 1;
        while (allCommands[y][b] != NULL) {
            free(allCommands[y][b]);
            b++;
        }
    }

}

// Exercise 1b: print service status
size_t sm_status(sm_status_t statuses[]) {
    int k = 0;
    int q = 0;
    while (k < process_num) {
        waitpid(my_PIDs[k][0], NULL, WNOHANG);
        status->pid = my_PIDs[k][0]; //assign pid
        status->path = my_paths[k]; //assign path
        int ret = kill(my_PIDs[k][0], 0);
        bool res = (ret == 0);
        status->running = res;
        statuses[q] = *status;
        q++;
        k++;
    }
    return process_num;
}

// Exercise 3: stop service, wait on service, and shutdown
void sm_stop(size_t index) {
    //send SIGTERM to all processes in a service,
    //wait on them until exit
    int ret, k = 1;
    while (my_PIDs[index][k] != -1) {
        ret = my_PIDs[index][k];
        kill(ret, SIGTERM);
        waitpid(ret, NULL, WNOHANG);
        k++;
        ret = 0;
    }
    ret = my_PIDs[index][0];
    kill(ret, SIGTERM);
    waitpid(ret, NULL, 0);
}

void sm_wait(size_t index) {
    //wait for all processes in a service
    int k = 1;
    while (my_PIDs[index][k] != -1) {
        waitpid(my_PIDs[index][k], NULL, 0);
        k++;
    }
    waitpid(my_PIDs[index][0], NULL, 0);
}

void sm_shutdown(void) {
    //just sm_stop on ALL SERVICES.
    for (int k = 0; k < process_num; k++) {
        sm_stop(k);
    }
}

// Exercise 4: start with output redirection
void sm_startlog(const char* processes[]) {
    sprintf(log_path, "service%d.log", process_num);
    //int open(const char* pathname, int flags);
    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0664);

    char* allCommands[SM_MAX_SERVICES][SM_MAX_PROCESSES];
    int x, y, a, b;
    a = 0;
    b = 0;
    bool end = false;
    //shove the processess into our 2D array for ease of processing.
    while (!end) {
        //check for the end of processes, which is 2 consecutive NULLs.
        if (processes[b] == NULL) { break; }
        x = 0; //reset x
        while (processes[b] != NULL) {
            allCommands[a][x] = (char*)malloc(sizeof(char*));
            strcpy(allCommands[a][x], processes[b]);
            b++;
            x++;
        }
        allCommands[a][x] = NULL;
        b++;
        a++;
        //jump onto next inner array.
    }

    char* path = (char*)malloc(sizeof(char*));
    int childPID;
    int fd[a][2]; // collection of file descriptors

    char* p;

    for (y = 0; y < a; y++) {
        strcpy(path, allCommands[y][0]); 
        p = strtok(path, "/");
        while (p != NULL) {
            //splitting process ongoing
            strcpy(token, p);
            p = strtok(NULL, "/");
        }
        strcpy(path, allCommands[y][0]);
        free(allCommands[y][0]);
        allCommands[y][0] = token;
        pipe(fd[y]);
        childPID = fork();

        if (childPID == 0) {
            //this is Child Process
            if (y != 0) {
                //there's commands before, read from pipe
                dup2(fd[y - 1][0], 0);
                close(fd[y - 1][0]);
                close(fd[y - 1][1]);
            }

            if (y != a - 1) {
                dup2(fd[y][1], 1);
                close(fd[y][0]);
                close(fd[y][1]);
            }

            if (y == a - 1) {
                //the LAST process(or only process) has its
                //stdout and error redirected to a file serviceN.log
                dup2(log_fd, 2);
                dup2(log_fd, 1);
                close(log_fd);
            }
            execv(path, allCommands[y]);
        }
        else {
            my_PIDs[process_num][y + 1] = childPID;
            if (y != 0) {
                close(fd[y - 1][0]);
                close(fd[y - 1][1]);
            }
            waitpid(childPID, NULL, WNOHANG);
        }
    }
    my_PIDs[process_num][0] = childPID;
    my_PIDs[process_num][a] = -1;
    my_paths[process_num] = path;
    process_num++;

    for (x = 0; x < a; x++) {
        close(fd[x][0]);
        close(fd[x][1]);
    }

    for (y = 0; y < a; y++) {
        b = 1;
        while (allCommands[y][b] != NULL) {
            free(allCommands[y][b]);
            b++;
        }
    }

}

// Exercise 5: show log file
void sm_showlog(size_t index) {
    sprintf(log_path, "service%ld.log", index);
    FILE *file;
    file = fopen(log_path, "r");
    if (file != NULL) {
        char z = fgetc(file);
        while (z != EOF) {
            putchar(z);
            z = fgetc(file); //get next character from the specified stream 
            //and advances the position indicator for the stream.
        }
    }
    else {
        printf("service has no log file\n");
    }
}