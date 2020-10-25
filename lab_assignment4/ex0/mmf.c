/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/

#include "mmf.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

void* mmf_create_or_open(const char* name, size_t sz) {
    /* TODO */
    //create or open the file (open)
    //O_EXCL
    int fd = open(name, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd == -1) {
        perror("create or open file descriptor error");
        exit(1);
    }

    //resize the file to desired size (ftruncate)
    ftruncate(fd, sz);

    //map the file into memory    
    //void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    char* data = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED_VALIDATE, fd, 0);
    close(fd);
    return data;
}

void mmf_close(void *ptr, size_t sz) {
    /* TODO */
    //unmap files
    munmap(ptr, sz);
}
