/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/



#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "mmf.h"

void* mmf_create_or_open(const char* name, size_t sz) {
    /* TODO */
    int fd = open(name, O_RDWR | O_CREAT, 0777);
    if (fd == -1) {
        perror("mmf_create_or_open open file descriptor error");
        exit(1);
    }
    ftruncate(fd, sz);
    char* mmf = mmap(NULL, sz, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    close(fd);
    return mmf;
}

void mmf_close(void* ptr, size_t sz) {
    /* TODO */
    munmap(ptr, sz);
}
