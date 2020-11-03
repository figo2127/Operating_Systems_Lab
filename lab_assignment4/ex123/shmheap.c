/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <fcntl.h> 
#include <sys/shm.h> 
#include <sys/stat.h> 
#include <sys/mman.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>

#include "shmheap.h"


sem_t mutex;

//For ex1, you just need to return a pointer to the allocated space in the shared heap.
//Once returned, the runner can use this allocated space in the same way you would use
//the space allocated by malloc, for example.
//To transform this pointer into an object handle, the runner calls shmheap_ptr_to_handle.
//This object handle can be transferred among processes.Hence you need to find a way to
//make the object handle independent of the memory space of a specific process


//order:
//shmheap_create -> shmheap_connect -> shmheap_alloc -> shmheap_ptr_to_handle -> shmheap_destroy
shmheap_memory_handle shmheap_create(const char* name, size_t len) {
    /* TODO */
    struct stat info;
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shmheap_create shm_open error\n");
        exit(1);
    }
    if (ftruncate(fd, len) == -1) {
        perror("truncate size error");
        exit(1);
    }
    if (fstat(fd, &info) == -1) {
        perror("get info error");
        exit(1);
    }
    void* mmf = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0); //Memory mapped file
    if (mmf == MAP_FAILED) {
        perror("map files failed");
    }
    shmheap_memory_handle* hdl_ptr = mmf;
    hdl_ptr->total_size = len;
    hdl_ptr->used_space = 0;
    hdl_ptr->baseaddr = mmf;
    hdl_ptr->init_offset = sizeof(char*) + 3 * sizeof(size_t) + sizeof(sem_t);
    sem_init(&(hdl_ptr->shmheap_mutex), 0, 1);
    book_keeper* init = (book_keeper*)((char*)hdl_ptr->baseaddr + hdl_ptr->init_offset);
    init->occupied = 0;
    init->prev_sz = 0;
    init->sz = (int)len;
    init->last_header = 1;
    return *hdl_ptr;

    /*close(fd);
    char* temp = (char*)malloc(sizeof(char) * strlen(name));
    strcpy(temp, name);
    shmheap_memory_handle res;
    res.name = temp;
    res.start_ptr = mmf;
    res.curr_ptr = mmf;
    res.size = len;
    res.fd = fd;
    printf("create: heap size: %ld, start addr: %p\n", heap_size, res.start_ptr);
    return res;*/
}

shmheap_memory_handle shmheap_connect(const char* name) {
    /* TODO */
    struct stat sb;
    shmheap_memory_handle* mem;
    char* temp = (char*)malloc(sizeof(char) * strlen(name));
    strcpy(temp, name);
    int fd = shm_open(name, O_RDWR, 0666);
    if (fstat(fd, &sb) == -1) {
        perror("fstat in connect");
        exit(1);
    }
    void* ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failure");
        exit(1);
    }
    mem = ptr;
    /*mem.name = temp;
    mem.size = sb.st_size;
    mem.start_ptr = ptr;
    mem.curr_ptr = ptr;
    mem.fd = fd;*/
    printf("connect: heap size: %ld, start addr: %p\n", sb.st_size, ptr);
    return *mem;
}

void shmheap_disconnect(shmheap_memory_handle mem) {
    /* TODO */
    //can straight away pass the address of mem into sem_wait via &mem
    shmheap_memory_handle* hdlptr = &mem;

    sem_wait(&(hdlptr->shmheap_mutex));
    //CS
    if (munmap(&hdlptr, sizeof(shmheap_memory_handle)) == -1) {
        perror("delete mappings failed");
        exit(1);
    }
    sem_post(&(hdlptr->shmheap_mutex));


    /*
    mem.size = NULL;
    mem.fd = NULL;*/

}

void shmheap_destroy(const char* name, shmheap_memory_handle mem) {
    /* TODO */
    shmheap_memory_handle* hdlptr = &mem;

    sem_destroy(&(hdlptr->shmheap_mutex));
    if (munmap(&mem, sizeof(shmheap_memory_handle)) == -1) {
        perror("delete mappings failed");
        exit(1);
    }
    shm_unlink(name);
}


void* shmheap_underlying(shmheap_memory_handle mem) {
    /* TODO */
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    //printf("Base address(shmheap) = %p\n", mem.baseaddr);
    sem_post(&(hdlptr->shmheap_mutex));
    return mem.baseaddr;
}

//the shmheap_memory_handle is first created by shmheap_create, then passed to shmheap_alloc
void* shmheap_alloc(shmheap_memory_handle mem, size_t sz) {
    /* TODO */
    //printf("allocating...\n");
    //void* res = mem.curr_ptr; //create a pointer here
    //printf("pointer at: %p\n", res);
    ////allocates an object of given size on the given shared heap, returns a pointer to the object that was allocated.
    //mem.curr_ptr += sz; //advances the curr_ptr of mem by sz units
    //printf("alloc address: %p curr_ptr: %p\n", res, mem.curr_ptr);
    //return res;
    //sem_wait(&shmheap_mutex);
    shmheap_memory_handle* hdlptr = (shmheap_memory_handle*)mem.baseaddr;
    sem_wait(&(hdlptr->shmheap_mutex));
    if (sz % 8 != 0) {
        sz = ((sz + 7) & (-8));
    }
    book_keeper* init = (book_keeper*)((char*)mem.baseaddr + mem.init_offset);
    
    while (1) {
        if (hdlptr->total_size - hdlptr->used_space < sz) {
            printf("total space = %ld, used space = %ld, size = %ld\n", hdlptr->total_size, hdlptr->used_space, sz);
            printf("%s\n", "No space left on memory");
            break;
        }
        else if ((size_t)init->sz == hdlptr->total_size) { //empty heap
            init->occupied = 1;
            init->sz = (int)sz;
            init->last_header = 1;
            hdlptr->used_space = hdlptr->used_space + (size_t)init->sz + sizeof(*init);
            break;
        }
        else if ((size_t)init->sz == 0) {
            init->occupied = 1;
            init->sz = (int)sz;
            init->last_header = 1;
            hdlptr->used_space = hdlptr->used_space + (size_t)init->sz + sizeof(*init);
            size_t free_space = hdlptr->total_size - hdlptr->used_space;
            if (free_space <= sizeof(book_keeper)) {
                init->sz = init->sz + (int)free_space;
            }
            break;
        }
        else if (init->occupied < 1 && init->last_header == 1) {
            hdlptr->used_space = hdlptr->used_space - ((size_t)init->sz);
            init->occupied = 1;
            init->sz = (int)sz;
            init->last_header = 1;
            hdlptr->used_space = hdlptr->used_space + sz;
            size_t free_space = hdlptr->total_size - hdlptr->used_space;
            if (free_space <= sizeof(book_keeper)) {
                init->sz = init->sz + (int)free_space;
            }
            break;
        }
        else if (init->occupied < 1 && (size_t)init->sz >= sz) { //first slot free and match
            init->occupied = 1;
            int oldsz = init->sz;
            if ((init->sz - (int)sz) > (int)sizeof(*init)) {
                init->sz = (int)sz;
                book_keeper* header = (book_keeper*)((char*)init + sizeof(*init) + sz);
                header->last_header = init->last_header;
                header->occupied = 0;
                header->prev_sz = init->sz;
                header->sz = oldsz - (int)sz - (int)sizeof(*header);
                book_keeper* next_header = (book_keeper*)((char*)header + sizeof(*header) + (size_t)header->sz);
                if (next_header->sz != 0) {
                    next_header->prev_sz = header->sz;
                }
                sem_post(&(hdlptr->shmheap_mutex));
                shmheap_free(mem, (char*)header + sizeof(*header));
                sem_wait(&(hdlptr->shmheap_mutex));
            }
            //init->sz=sz;
            //hdlptr->used_space= hdlptr->used_space + (size_t)init->sz + sizeof(*init);
            break;
        }
        else { //first slot is not free
            size_t curr_size = (size_t)init->sz;
            init->last_header = 0;
            init = (book_keeper*)((char*)init + curr_size + sizeof(*init));
            init->prev_sz = (int)curr_size;
            continue;
        }
    }
    //sem_post(&shmheap_mutex);
    sem_post(&(hdlptr->shmheap_mutex));
    return (book_keeper*)((char*)init + sizeof(*init));
}

//needs changes
void shmheap_free(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    shmheap_memory_handle* hdlptr = (shmheap_memory_handle*)mem.baseaddr;
    sem_wait(&(hdlptr->shmheap_mutex));
    book_keeper* header = (book_keeper*)((char*)ptr - sizeof(book_keeper));
    size_t sz = (size_t)header->sz;
    header->occupied = 0;
    //printf("Cuurent header details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", header, header->sz, header->prev_sz, header->occupied, header->last_header);
    //hdlptr->used_space=hdlptr->used_space - sizeof(book_keeper) - (size_t)header->sz;
    //Get next header
    book_keeper* next_header = (book_keeper*)((char*)ptr + sz);
    //printf("Next header initial details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", next_header, next_header->sz, next_header->prev_sz, next_header->occupied, next_header->last_header);
    if (next_header->occupied < 1 && header->last_header != 1) {
        header->sz = (int)sz + next_header->sz + (int)sizeof(book_keeper);
        if (next_header->last_header == 1) {
            header->last_header = 1;
            //printf("New current header details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", header, header->sz, header->prev_sz, header->occupied, header->last_header);
        }
        else {
            //have to change next next header's previous size
            book_keeper* next_next_header = (book_keeper*)((char*)next_header + next_header->sz + sizeof(book_keeper));
            next_next_header->prev_sz = (int)header->sz;
            //printf("Next next header initial details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", next_next_header, next_next_header->sz, next_next_header->prev_sz, next_next_header->occupied, next_next_header->last_header);
        }
    }
    if ((size_t)header->prev_sz > 0) {
        size_t prev_sz = (size_t)header->prev_sz;
        book_keeper* prev_header = (book_keeper*)((char*)header - prev_sz - sizeof(book_keeper));
        //printf("Previous header details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", prev_header, prev_header->sz, prev_header->prev_sz, prev_header->occupied, prev_header->last_header);
        if (prev_header->occupied < 1) {
            prev_header->sz = header->sz + prev_header->sz + (int)sizeof(book_keeper);
        }
        if (header->last_header == 1) {
            prev_header->last_header = 1;
            header->last_header = 0;
        }
        //printf("Previous header new details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", prev_header, prev_header->sz, prev_header->prev_sz, prev_header->occupied, prev_header->last_header);
    }
    //sem_post(&shmheap_mutex);
    sem_post(&(hdlptr->shmheap_mutex));
}

shmheap_object_handle shmheap_ptr_to_handle(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    shmheap_object_handle* objhand = (shmheap_object_handle*)malloc(sizeof(shmheap_object_handle));
    int p = (char*)ptr - (char*)mem.baseaddr;
    objhand->offset = p;
    sem_post(&(hdlptr->shmheap_mutex));
    return *objhand;
    /*shmheap_object_handle obj;
    obj.offset = ptr - mem.start_ptr;
    printf("offset is %ld\n", obj.offset);
    return obj;*/
}

void* shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl) {
    /* TODO */
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    void* baseaddr = mem.baseaddr;
    int offset = hdl.offset;
    char* target = (char*)baseaddr + offset;
    //sem_post(&shmheap_mutex);
    sem_post(&(hdlptr->shmheap_mutex));
    return target;
    /*void* res;
    res = mem.start_ptr + hdl.offset;
    printf("start_addr: %p res addr: %p\n", mem.start_ptr, res);
    return res;*/
}