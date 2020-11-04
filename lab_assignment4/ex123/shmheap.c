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
    shmheap_memory_handle* hdlptr = mmf;
    hdlptr->total_size = len;
    hdlptr->used_space = hdl_sz;
    hdlptr->baseaddr = hdlptr;
    hdlptr->len = len;
    hdlptr->init_offset = hdl_sz;
    //initialize semaphore
    sem_init(&(hdlptr->shmheap_mutex), 0, 1);

    //initialize book keeper
    book_keeper* bk = (book_keeper*)((char*)hdlptr->baseaddr + (size_t)hdlptr->init_offset);
    bk->occupied = 0;
    bk->prev_sz = 0;
    bk->sz = (int)len;
    bk->bk_prev = 1;
    return *hdlptr;

}

shmheap_memory_handle shmheap_connect(const char* name) {
    /* TODO */
    //sem_wait(&shmheap_mutex);
    struct stat info;
    //grant read permission, write permission
    int fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open error");
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
    shmheap_memory_handle* hdlptr = ptr;
  
    //printf("connect: heap size: %ld, start addr: %p\n", info.st_size, ptr);

    return *hdlptr;
}

void shmheap_disconnect(shmheap_memory_handle mem) {
    /* TODO */
     //unmaps the shared heap
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    void* addr = hdlptr->baseaddr;
    size_t sz = hdlptr->len;
    if (munmap(addr, sz) == -1) {
        perror("un mappings failed");
    }
    sem_post(&(hdlptr->shmheap_mutex));
}

void shmheap_destroy(const char* name, shmheap_memory_handle mem) {
    /* TODO */
    //unmaps the shared heap + unlinks(delete) the shared memory with the given name.
    shmheap_memory_handle* hdlptr = &mem;
    sem_destroy(&(hdlptr->shmheap_mutex));
    void* addr = hdlptr->baseaddr;
    size_t sz = hdlptr->len;
    if (munmap(addr, sz) == -1) {
        perror("un mappings failed");
    }
    shm_unlink(name);

}

void* shmheap_underlying(shmheap_memory_handle mem) {
    /* TODO */
    //shmheap_memory_handle* hdlptr = &mem;
    //sem_wait(&(hdlptr->shmheap_mutex));
    ////printf("Base address(shmheap) = %p\n", mem.baseaddr);
    //sem_post(&(hdlptr->shmheap_mutex));
    return mem.baseaddr;

    //return &mem;
}


void* shmheap_alloc(shmheap_memory_handle mem, size_t sz) { //can just return base address from mem
    /* TODO */
    shmheap_memory_handle* hdlptr = shmheap_underlying(mem);
    sem_wait(&(hdlptr->shmheap_mutex));
    size_t modified_sz;
    modified_sz = ensure_eightbyte_aligned(sz);

    book_keeper* bk = (book_keeper*)((char*)mem.baseaddr + (size_t)mem.init_offset);

    while (1) {
        if ((size_t)bk->sz == hdlptr->total_size) { 
            bk->sz = (int)modified_sz;
            bk->occupied = 1;
            bk->bk_prev = 1;
            hdlptr->used_space = hdlptr->used_space + (size_t)bk->sz + sizeof(*bk);
            break;
        }
        else if ((size_t)bk->sz == 0 && bk->bk_prev == 0 && bk->occupied == 0) {
            bk->occupied = 1;
            bk->sz = (int)modified_sz;
            bk->bk_prev = 0;
            hdlptr->used_space = hdlptr->used_space + (size_t)bk->sz + sizeof(*bk);
            char* endOfAddr = (char*)bk + sizeof(*bk) + modified_sz;
            size_t space_used = endOfAddr - (char*)hdlptr->baseaddr;
            size_t space_left = hdlptr->total_size - space_used;
            if (space_left <= sizeof(book_keeper)) { 
                bk->sz += (int)space_left;
                bk->bk_prev = 1;
            }
            else {
                book_keeper* header = (book_keeper*)((char*)bk + sizeof(*bk) + modified_sz);
                header->bk_prev = 1;
                header->occupied = 0;
                header->prev_sz = bk->sz;
                header->sz = 0;
            }
            break;
        }
        else if (bk->occupied < 1 && bk->bk_prev == 1) {
            hdlptr->used_space = hdlptr->used_space - ((size_t)bk->sz);
            bk->occupied = 1;
            bk->sz = (int)modified_sz;
            bk->bk_prev = 0;
            hdlptr->used_space = hdlptr->used_space + modified_sz;
            char* end = (char*)bk + sizeof(*bk) + modified_sz;
            char* beg = (char*)hdlptr->baseaddr;
            size_t used = end - beg;
            size_t free = hdlptr->total_size - used;
            if (free <= sizeof(book_keeper)) {
                bk->sz = bk->sz + (int)free;
                bk->bk_prev = 1;
            }
            else {
                book_keeper* header = (book_keeper*)((char*)bk + sizeof(*bk) + modified_sz);
                header->bk_prev = 1;
                header->occupied = 0;
                header->prev_sz = bk->sz;
                header->sz = 0;
            }
            break;
        }
        else if (bk->occupied < 1 && (size_t)bk->sz >= modified_sz) { //first slot free and match
            bk->occupied = 1;
            int oldsz = bk->sz;
            if ((bk->sz - (int)modified_sz) > (int)sizeof(*bk)) {
                bk->sz = (int)modified_sz;
                book_keeper* header = (book_keeper*)((char*)bk + sizeof(*bk) + modified_sz);
                header->bk_prev = bk->bk_prev;
                header->occupied = 0;
                header->prev_sz = bk->sz;
                header->sz = oldsz - (int)modified_sz - (int)sizeof(*header);
                book_keeper* next_header = (book_keeper*)((char*)header + sizeof(*header) + (size_t)header->sz);
                if (next_header->sz != 0) {
                    next_header->prev_sz = header->sz;
                }
                sem_post(&(hdlptr->shmheap_mutex));
                shmheap_free(mem, (char*)header + sizeof(*header));
                sem_wait(&(hdlptr->shmheap_mutex));
            }
            //bk->sz=sz;
            hdlptr->used_space = hdlptr->used_space + (size_t)bk->sz + sizeof(*bk);
            break;
        }
        else { //first slot is not free
            size_t curr_size = (size_t)bk->sz;
            bk->bk_prev = 0;
            bk = (book_keeper*)((char*)bk + curr_size + sizeof(*bk));
            bk->prev_sz = (int)curr_size;
            continue;
        }
    }
    //sem_post(&shmheap_mutex);
    sem_post(&(hdlptr->shmheap_mutex));
    return (book_keeper*)((char*)bk + sizeof(*bk));

}

size_t ensure_eightbyte_aligned(size_t sz) {
    if (sz % 8 != 0) {
        sz = ((sz + 7) & (-8)); //bitwise AND
    }
    return sz;
}

void shmheap_free(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    //sem_wait(&shmheap_mutex);
    shmheap_memory_handle* hdlptr = (shmheap_memory_handle*)mem.baseaddr;
    sem_wait(&(hdlptr->shmheap_mutex));
    book_keeper* header = (book_keeper*)((char*)ptr - sizeof(book_keeper));
    size_t sz = (size_t)header->sz;
    header->occupied = 0;
    //Get next header
    book_keeper* next_header = (book_keeper*)((char*)ptr + sz);

    //attempt to combine free spaces
    if (next_header->occupied < 1 && header->bk_prev != 1) {
        header->sz = (int)sz + next_header->sz + (int)sizeof(book_keeper);
        if (next_header->bk_prev != 1) {
            //have to change next next header's previous size
            book_keeper* next_next_header = (book_keeper*)((char*)next_header + next_header->sz + sizeof(book_keeper));
            next_next_header->prev_sz = (int)header->sz;
        }
        else {
            header->bk_prev = 1;
        }
    }
    if ((size_t)header->prev_sz > 0) {
        size_t prev_sz = (size_t)header->prev_sz;
        book_keeper* prev_header = (book_keeper*)((char*)header - prev_sz - sizeof(book_keeper));
        if (prev_header->occupied < 1) {
            prev_header->sz = header->sz + prev_header->sz + (int)sizeof(book_keeper);
        }
        if (header->bk_prev == 1) {
            prev_header->bk_prev = 1;
            header->bk_prev = 0;
        }
    }
    //sem_post(&shmheap_mutex);
    sem_post(&(hdlptr->shmheap_mutex));
}

shmheap_object_handle shmheap_ptr_to_handle(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    shmheap_object_handle* objhand = (shmheap_object_handle*)malloc(sizeof(shmheap_object_handle));
    int p = (char*)ptr - (char*)mem.baseaddr;
    objhand->offset = p;
    return *objhand;
}

void* shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl) {
    /* TODO */
    void* baseaddr = mem.baseaddr;
    int offset = hdl.offset;
    char* target = (char*)baseaddr + offset;
    return target;
}
