/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/
#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <sys/shm.h> 
#include <sys/stat.h> 
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include "shmheap.h"

size_t heap_size;
//For ex1, you just need to return a pointer to the allocated space in the shared heap.
//Once returned, the runner can use this allocated space in the same way you would use
//the space allocated by malloc, for example.
//To transform this pointer into an object handle, the runner calls shmheap_ptr_to_handle.
//This object handle can be transferred among processes.Hence you need to find a way to
//make the object handle independent of the memory space of a specific process




//order:
//shmheap_create -> shmheap_alloc -> shmheap_ptr_to_handle -> shmheap_destroy
shmheap_memory_handle shmheap_create(const char* name, size_t len) {
    /* TODO */
    heap_size = len;
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shmheap_create shm_open error\n");
        exit(1);
    }
    ftruncate(fd, len);
    void* mmf = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED_VALIDATE, fd, 0); //Memory mapped file
    close(fd);
    char* temp = (char*)malloc(sizeof(char) * strlen(name));
    strcpy(temp, name);
    shmheap_memory_handle res;
    res.name = temp;
    res.start_ptr = mmf;
    res.curr_ptr = mmf;
    res.size = len;
    res.fd = fd;
    printf("create: heap size: %ld, start addr: %p\n", heap_size, res.start_ptr);
    return res;
}

shmheap_memory_handle shmheap_connect(const char* name) {
    /* TODO */
    struct stat sb;
    shmheap_memory_handle mem;
    char* temp = (char*)malloc(sizeof(char) * strlen(name));
    strcpy(temp, name);
    int fd = shm_open(name, O_RDWR, 0666);
    if (fstat(fd, &sb) == -1) {
        perror("fstat in connect");
        exit(EXIT_FAILURE);
    }
    void* ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0);
    mem.name = temp;
    mem.size = sb.st_size;
    mem.start_ptr = ptr;
    mem.curr_ptr = ptr;
    mem.fd = fd;
    printf("connect: heap size: %ld, start addr: %p\n", sb.st_size, ptr);
    return mem;
}

void shmheap_disconnect(shmheap_memory_handle mem) {
    /* TODO */
    mem.start_ptr = NULL;
    mem.size = NULL;
    mem.fd = NULL;

}

void shmheap_destroy(const char* name, shmheap_memory_handle mem) {
    /* TODO */
    munmap(mem.start_ptr, mem.size);
    shm_unlink(name);
}

void* shmheap_underlying(shmheap_memory_handle mem) {
    /* TODO */
}

//the shmheap_memory_handle is first created by shmheap_create, then passed to shmheap_alloc
void* shmheap_alloc(shmheap_memory_handle mem, size_t sz) {
    /* TODO */
    printf("allocating...\n");
    void* res = mem.curr_ptr; //create a pointer here
    printf("pointer at: %p\n", res);
    //allocates an object of given size on the given shared heap, returns a pointer to the object that was allocated.
    mem.curr_ptr += sz; //advances the curr_ptr of mem by sz units
    printf("alloc address: %p curr_ptr: %p\n", res, mem.curr_ptr);
    return res;
}

void shmheap_free(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
}

shmheap_object_handle shmheap_ptr_to_handle(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    shmheap_object_handle obj;
    obj.offset = ptr - mem.start_ptr;
    printf("offset is %ld\n", obj.offset);
    return obj;
}

void* shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl) {
    /* TODO */
    void* res;
    res = mem.start_ptr + hdl.offset;
    printf("start_addr: %p res addr: %p\n", mem.start_ptr, res);
    return res;
}