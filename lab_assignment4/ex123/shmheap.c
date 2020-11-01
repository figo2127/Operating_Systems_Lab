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

shmheap_memory_handle shmheap_create(const char* name, size_t len) {
    /* TODO */
    heap_size = len;
    int fd = shm_open(name, O_CREAT | O_RDWR, 0777);
    if (fd == -1) {
        perror("shmheap_create shm_open error\n");
        exit(1);
    }
    ftruncate(fd, len);
    void* mmf = mmap(NULL, len, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    close(fd);
    shmheap_memory_handle res;
    char* temp = (char*)malloc(sizeof(char) * strlen(name));
    strcpy(temp, name);
    res.name = temp;
    res.start_ptr = mmf;
    res.curr_ptr = mmf;
    res.size = len;
    res.fd = fd;
    printf("create: healp size: %ld, start addr: %p\n", heap_size, res.start_ptr);
    return res;
}

shmheap_memory_handle shmheap_connect(const char* name) {
    /* TODO */
    struct stat sb;
    shmheap_memory_handle mem;
    char* temp = (char*)malloc(sizeof(char) * strlen(name));
    strcpy(temp, name);
    int fd = shm_open(name, O_RDWR, 0777);
    if (fstat(fd, &sb) == -1) {
        perror("fstat in connect");
        exit(EXIT_FAILURE);
    }
    void* ptr = mmap(NULL, sb.st_size, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
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

void* shmheap_alloc(shmheap_memory_handle mem, size_t sz) {
    /* TODO */
    printf("in alloc\n");
    void* res = mem.curr_ptr;
    mem.curr_ptr += sz;
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