/*************************************
* Lab 4
* Name: 
* Student No:
* Lab Group:
*************************************/

#include "shmheap.h"


sem_t mutex;
size_t hdl_sz = (sizeof(char*) + 3 * sizeof(size_t) + sizeof(sem_t));


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
    int fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shmheap_create shm_open error\n");
    }
    if (ftruncate(fd, len) == -1) {
        perror("truncate size error");
    }
    //get size of shared memory
    if (fstat(fd, &info) == -1) {
        perror("get info error");
    }
    void* mmf = mmap(NULL, info.st_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0); //Memory mapped file
    if (mmf == MAP_FAILED) {
        perror("map files failed");
    }
    shmheap_memory_handle* handle_ptr = mmf;
    handle_ptr->total_size = len;
    handle_ptr->used_space = hdl_sz;
    handle_ptr->baseaddr = handle_ptr;
    handle_ptr->length = len;
    handle_ptr->init_offset = hdl_sz;
    //initialize semaphore
    sem_init(&(handle_ptr->shmheap_mutex), 0, 1);

    //initialize book keeper
    book_keeper* bk = (book_keeper*)((char*)handle_ptr->baseaddr + (size_t)handle_ptr->init_offset);
    bk->occupied = 0;
    bk->prev_sz = 0;
    bk->sz = (int)len;
    bk->bk_prev = 1;
    return *handle_ptr;

}

shmheap_memory_handle shmheap_connect(const char* name) {
    /* TODO */
    //sem_wait(&shmheap_mutex);
    struct stat info;
    //grant read permission, write permission
    int fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open error");
        //exit(1);
    }
    //get size of shared memory
    if (fstat(fd, &info) == -1) {
        perror("fstat in connect");
        //exit(1);
    }

    //map the shared memory
    void* ptr = mmap(NULL, info.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failure");
        //exit(1);
    }
    shmheap_memory_handle* handle_ptr = ptr;
  
    //printf("connect: heap size: %ld, start addr: %p\n", info.st_size, ptr);

    return *handle_ptr;
}

void shmheap_disconnect(shmheap_memory_handle mem) {
    /* TODO */
     //unmaps the shared heap
    shmheap_memory_handle* handle_ptr = &mem;
    sem_wait(&(handle_ptr->shmheap_mutex));
    void* addr = handle_ptr->baseaddr;
    size_t sz = handle_ptr->length;
    if (munmap(addr, sz) == -1) {
        perror("un mappings failed");
    }
    sem_post(&(handle_ptr->shmheap_mutex));
}

void shmheap_destroy(const char* name, shmheap_memory_handle mem) {
    /* TODO */
    //unmaps the shared heap + unlinks(delete) the shared memory with the given name.
    shmheap_memory_handle* handle_ptr = &mem;
    sem_destroy(&(handle_ptr->shmheap_mutex));
    void* addr = handle_ptr->baseaddr;
    size_t sz = handle_ptr->length;
    if (munmap(addr, sz) == -1) {
        perror("un mappings failed");
        //exit(1);
    }
    shm_unlink(name);

}

void* shmheap_underlying(shmheap_memory_handle mem) {
    /* TODO */
    return mem.baseaddr;

    //return &mem;
}


void* shmheap_alloc(shmheap_memory_handle mem, size_t sz) { //can just return base address from mem
    /* TODO */
    shmheap_memory_handle* handle_ptr = shmheap_underlying(mem);
    sem_wait(&(handle_ptr->shmheap_mutex));
    size_t modified_sz;
    modified_sz = ensure_eightbyte_aligned(sz);

    book_keeper* bk = (book_keeper*)((char*)mem.baseaddr + (size_t)mem.init_offset);

    while (1) {
        if (bk->sz == handle_ptr->total_size) { 
            bk->occupied = 1;
            bk->sz = modified_sz;
            bk->bk_prev = 1;
            handle_ptr->used_space = handle_ptr->used_space + bk->sz + sizeof(*bk);
            break;
        }
        else if (bk->occupied != 1 && bk->bk_prev == 1) {
            handle_ptr->used_space = handle_ptr->used_space - (bk->sz);
            bk->occupied = 1;
            bk->sz = modified_sz;
            bk->bk_prev = 0;
            handle_ptr->used_space = handle_ptr->used_space + modified_sz;
            char* endOfAddr = (char*)bk + sizeof(*bk) + modified_sz;
            size_t space_used = endOfAddr - (char*)handle_ptr->baseaddr;
            size_t space_left = handle_ptr->total_size - space_used;
            if (space_left <= sizeof(book_keeper)) {
                bk->sz = bk->sz + space_left;
                bk->bk_prev = 1;
            }
            else {
                book_keeper* heading = (book_keeper*)((char*)bk + sizeof(*bk) + modified_sz);
                heading->occupied = 0;
                heading->bk_prev = 1;
                heading->prev_sz = bk->sz;
                heading->sz = 0;
            }
            break;
        }
        else if (bk->sz == 0 && bk->bk_prev == 0 && bk->occupied == 0) {
            bk->occupied = 1;
            bk->sz = modified_sz;
            bk->bk_prev = 0;
            handle_ptr->used_space = handle_ptr->used_space + bk->sz + sizeof(*bk);
            char* endOfAddr = (char*)bk + sizeof(*bk) + modified_sz;
            size_t space_used = endOfAddr - (char*)handle_ptr->baseaddr;
            size_t space_left = handle_ptr->total_size - space_used;
            if (space_left <= sizeof(book_keeper)) {
                bk->sz += space_left;
                bk->bk_prev = 1;
            }
            else {
                book_keeper* heading = (book_keeper*)((char*)bk + sizeof(*bk) + modified_sz);
                heading->occupied = 0;
                heading->bk_prev = 1;
                heading->prev_sz = bk->sz;
                heading->sz = 0;
            }
            break;
        }
        //first fit
        else if (bk->occupied != 1 && bk->sz >= modified_sz) { 
            bk->occupied = 1;
            int oldsz = bk->sz;
            if ((bk->sz - modified_sz) > (int)sizeof(*bk)) {
                bk->sz = modified_sz;
                book_keeper* heading = (book_keeper*)((char*)bk + sizeof(*bk) + modified_sz);
                heading->occupied = 0;
                heading->bk_prev = bk->bk_prev;
                heading->prev_sz = bk->sz;
                heading->sz = oldsz - modified_sz - sizeof(*heading);
                book_keeper* next_heading = (book_keeper*)((char*)heading + sizeof(*heading) + heading->sz);
                if (next_heading->sz != 0) {
                    next_heading->prev_sz = heading->sz;
                }
                sem_post(&(handle_ptr->shmheap_mutex));
                shmheap_free(mem, (char*)heading + sizeof(*heading));
                sem_wait(&(handle_ptr->shmheap_mutex));
            }
            handle_ptr->used_space = handle_ptr->used_space + (bk->sz) + sizeof(*bk);
            break;
        }
        else { //first slot is not free
            size_t curr_size = bk->sz;
            bk->bk_prev = 0;
            bk = (book_keeper*)((char*)bk + curr_size + sizeof(*bk));
            bk->prev_sz = curr_size;
            continue;
        }
    }
    sem_post(&(handle_ptr->shmheap_mutex));
    return (book_keeper*)((char*)bk + sizeof(*bk));
}

size_t ensure_eightbyte_aligned(size_t sz) {
    if (sz % 8 != 0) {
        sz = ((-8) & (sz + 7)); //bitwise AND
    }
    return sz;
}

void shmheap_free(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    shmheap_memory_handle* handle_ptr = (shmheap_memory_handle*)mem.baseaddr;
    sem_wait(&(handle_ptr->shmheap_mutex));
    book_keeper* heading = (book_keeper*)((char*)ptr - sizeof(book_keeper));
    heading->occupied = 0;
    size_t sz = heading->sz;

    book_keeper* next_heading = (book_keeper*)((char*)ptr + sz);

    //attempt to combine free spaces
    if (next_heading->occupied != 1 && heading->bk_prev != 1) {
        heading->sz = sz + next_heading->sz + sizeof(book_keeper);
        if (next_heading->bk_prev != 1) {
            //have to change the next next heading's previous size
            book_keeper* next_next_heading = (book_keeper*)((char*)next_heading + next_heading->sz + sizeof(book_keeper));
            next_next_heading->prev_sz = heading->sz;
        }
        else {
            heading->bk_prev = 1;
        }
    }
    if (heading->prev_sz > 0) {
        size_t prev_sz = heading->prev_sz;
        book_keeper* prev_header = (book_keeper*)((char*)heading - prev_sz - sizeof(book_keeper));
        if (prev_header->occupied < 1) {
            prev_header->sz = heading->sz + prev_header->sz + sizeof(book_keeper);
        }
        if (heading->bk_prev == 1) {
            prev_header->bk_prev = 1;
            heading->bk_prev = 0;
        }
    }
    sem_post(&(handle_ptr->shmheap_mutex));
}

shmheap_object_handle shmheap_ptr_to_handle(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    shmheap_object_handle* res = (shmheap_object_handle*)malloc(sizeof(shmheap_object_handle));
    int p = (char*)ptr - (char*)mem.baseaddr;
    res->offset = p;
    return *res;
}

void* shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl) {
    /* TODO */
    void* baseaddr = mem.baseaddr;
    char* res = (char*)baseaddr + hdl.offset;
    return res;
}
