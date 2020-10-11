/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * Your implementation should go in this file.
 */
#include "fizzbuzz_workers.h"
#include "barrier.h"
#include <stdio.h>

#define ensure_successful_malloc(ptr)                           \
  if (ptr == NULL) {                                            \
    perror("Memory allocation unsuccessful for" #ptr "\n");     \
    exit(1);                                                    \
  }

// declare variables to be used here
int num_threads;
barrier_t *barrier;

void fizzbuzz_init(int n) {
    barrier = malloc(sizeof(barrier_t));
    ensure_successful_malloc(barrier);
    barrier_init(barrier, 4); //we need 4 because there are in total 4 cases,
                               //and always one of them will trigger the if statement
    num_threads = n;
}

void num_thread(int n, void (*print_num)(int)) {
    for(int a = 1; a <= n; a++){
        if ((a % 3) && (a % 5)) {
            print_num(a); //only print those numbers that match requirement
        }
        barrier_wait(barrier);
    }
}

void fizz_thread(int n, void (*print_fizz)(void)) {
    for (int a = 1; a <= n; a++){
        if ((a % 3 == 0) && (a % 5)) {
            print_fizz();
        }
        barrier_wait(barrier);
    }
}

void buzz_thread(int n, void (*print_buzz)(void)) {
    for (int a = 1; a <= n; a++) {
        if ((a % 3) && (a % 5 == 0)) {
            print_buzz();
        }
        barrier_wait(barrier);
    }
}

void fizzbuzz_thread(int n, void (*print_fizzbuzz)(void)) {
    for (int a = 1; a <= n; a++) {
        if ((a % 3 == 0) && (a % 5 == 0)) {
            print_fizzbuzz();
        }
        barrier_wait(barrier);
    }
}

void fizzbuzz_destroy() {
    barrier_destroy(barrier);
    free(barrier);
}
