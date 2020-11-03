/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/

#include <stddef.h>
#include <semaphore.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <fcntl.h> 
#include <pthread.h>
#include <sys/shm.h> 
#include <sys/stat.h> 
#include <sys/mman.h>
#include <sys/types.h>
#include <string.h>


/*
You should modify these structs to suit your implementation,
but remember that all the functions declared here must have
a signature that is callable using the APIs specified in the
lab document.

You may define other helper structs or convert the existing
structs to typedefs, as long as the functions satisfy the
requirements in the lab document.  If you declare additional names (helper structs or helper functions), they should be prefixed with "shmheap_" to avoid potential name clashes.
*/

#define MAX_FIRST_BOOK_KEEPING_SIZE = 80;
#define SUBSEQ_MAX = 16;

#define MAX_LEN 100

typedef struct { //should behave like a pointer to the shared memory region
    //char * name;
    void* baseaddr;
    //int len;
    //char buf[MAX_LEN];
   //void * target;
    int init_offset;
    int len;
    size_t total_size;
    size_t used_space;
    sem_t shmheap_mutex;
    //variable to check if there is a first header anot
} shmheap_memory_handle;

typedef struct {
    int offset;
} shmheap_object_handle;

typedef struct {
    int occupied;
    int last_header;
    int sz;
    int prev_sz;
} shmheap_header;





/*
These functions form the public API of your shmheap library.
*/

shmheap_memory_handle shmheap_create(const char *name, size_t len);
shmheap_memory_handle shmheap_connect(const char *name);
void shmheap_disconnect(shmheap_memory_handle mem);
void shmheap_destroy(const char *name, shmheap_memory_handle mem);
void *shmheap_underlying(shmheap_memory_handle mem);
void *shmheap_alloc(shmheap_memory_handle mem, size_t sz);
void shmheap_free(shmheap_memory_handle mem, void *ptr);
shmheap_object_handle shmheap_ptr_to_handle(shmheap_memory_handle mem, void *ptr);
void *shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl);
