#include "shmheap.h"

shmheap_memory_handle shmheap_create(const char* name, size_t len) {
    /* TODO */
    shmheap_memory_handle* hdlptr; //possible to just make struct not pointer
    int fd;
    struct stat sb;


    //create a shm, map it and return the struct containing the pointer

    //open the shared memory 
    fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("shm_open");
    }
    if (ftruncate(fd, len) == -1)
    {
        perror("ftruncate");
    }
    //get size of shared memory
    if (fstat(fd, &sb) == -1)
    {
        perror("fstat");
    }

    //map the shared memory
    hdlptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); //save this as pointer then allocate to mem.base
    if (hdlptr == MAP_FAILED)
    {
        perror("mmap");
    }
    //hdlptr->name=strdup(name);
    hdlptr->total_size = len;
    hdlptr->used_space = 0;
    hdlptr->baseaddr = hdlptr;
    hdlptr->init_offset = sizeof(char*) + 3 * sizeof(size_t) + sizeof(sem_t);
    sem_init(&(hdlptr->shmheap_mutex), 0, 1);
    shmheap_header* init = (shmheap_header*)((char*)hdlptr->baseaddr + hdlptr->init_offset);
    init->occupied = 0;
    init->prev_sz = 0;
    init->sz = (int)len;
    init->last_header = 1;
    return *hdlptr;

}

shmheap_memory_handle shmheap_connect(const char* name) {
    /* TODO */
    //sem_wait(&shmheap_mutex);
    shmheap_memory_handle* hdlptr;
    int fd;
    struct stat sb;

    //open the shared memory
    fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("shm_open");
    }
    //get size of shared memory
    if (fstat(fd, &sb) == -1)
    {
        perror("fstat");
    }

    //map the shared memory
    hdlptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (hdlptr == MAP_FAILED)
    {
        perror("mmap");
    }
    //printf("%s %p\n", "memory handle base address is (connect)", hdlptr);
    //sem_post(&shmheap_mutex);
    return *hdlptr;


}

void shmheap_disconnect(shmheap_memory_handle mem) {
    /* TODO */
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    munmap(&mem, sizeof(shmheap_memory_handle));
    sem_post(&(hdlptr->shmheap_mutex));
}

void shmheap_destroy(const char* name, shmheap_memory_handle mem) {
    /* TODO */
    shmheap_memory_handle* hdlptr = &mem;
    sem_destroy(&(hdlptr->shmheap_mutex));
    munmap(&mem, sizeof(shmheap_memory_handle));
    shm_unlink(name);

}

void* shmheap_underlying(shmheap_memory_handle mem) {
    /* TODO */
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    //printf("Base address(shmheap) = %p\n", mem.baseaddr);
    sem_post(&(hdlptr->shmheap_mutex));
    return mem.baseaddr;

    //return &mem;
}

void* shmheap_alloc(shmheap_memory_handle mem, size_t sz) { //can just return base address from mem
    /* TODO */
    //sem_wait(&shmheap_mutex);
    shmheap_memory_handle* hdlptr = (shmheap_memory_handle*)mem.baseaddr;
    sem_wait(&(hdlptr->shmheap_mutex));
    if (sz % 8 != 0) {
        sz = ((sz + 7) & (-8));
    }
    shmheap_header* init = (shmheap_header*)((char*)mem.baseaddr + mem.init_offset);

    while (true) {
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
        else if ((size_t)init->sz == 0 && init->last_header == 0 && init->occupied == 0) {
            init->occupied = 1;
            init->sz = (int)sz;
            init->last_header = 1;
            hdlptr->used_space = hdlptr->used_space + (size_t)init->sz + sizeof(*init);
            size_t free_space = hdlptr->total_size - hdlptr->used_space;
            if (free_space <= sizeof(shmheap_header)) {
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
            if (free_space <= sizeof(shmheap_header)) {
                init->sz = init->sz + (int)free_space;
            }
            break;
        }
        else if (init->occupied < 1 && (size_t)init->sz >= sz) { //first slot free and match
            init->occupied = 1;
            int oldsz = init->sz;
            if ((init->sz - (int)sz) > (int)sizeof(*init)) {
                init->sz = (int)sz;
                shmheap_header* header = (shmheap_header*)((char*)init + sizeof(*init) + sz);
                header->last_header = init->last_header;
                header->occupied = 0;
                header->prev_sz = init->sz;
                header->sz = oldsz - (int)sz - (int)sizeof(*header);
                shmheap_header* next_header = (shmheap_header*)((char*)header + sizeof(*header) + (size_t)header->sz);
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
            init = (shmheap_header*)((char*)init + curr_size + sizeof(*init));
            init->prev_sz = (int)curr_size;
            continue;
        }
    }
    //sem_post(&shmheap_mutex);
    sem_post(&(hdlptr->shmheap_mutex));
    return (shmheap_header*)((char*)init + sizeof(*init));

}

void shmheap_free(shmheap_memory_handle mem, void* ptr) {
    /* TODO */
    //sem_wait(&shmheap_mutex);
    shmheap_memory_handle* hdlptr = (shmheap_memory_handle*)mem.baseaddr;
    sem_wait(&(hdlptr->shmheap_mutex));
    shmheap_header* header = (shmheap_header*)((char*)ptr - sizeof(shmheap_header));
    size_t sz = (size_t)header->sz;
    header->occupied = 0;
    //printf("Cuurent header details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", header, header->sz, header->prev_sz, header->occupied, header->last_header);
    //hdlptr->used_space=hdlptr->used_space - sizeof(shmheap_header) - (size_t)header->sz;
    //Get next header
    shmheap_header* next_header = (shmheap_header*)((char*)ptr + sz);
    //printf("Next header initial details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", next_header, next_header->sz, next_header->prev_sz, next_header->occupied, next_header->last_header);
    if (next_header->occupied < 1 && header->last_header != 1) {
        header->sz = (int)sz + next_header->sz + (int)sizeof(shmheap_header);
        if (next_header->last_header == 1) {
            header->last_header = 1;
            //printf("New current header details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", header, header->sz, header->prev_sz, header->occupied, header->last_header);
        }
        else {
            //have to change next next header's previous size
            shmheap_header* next_next_header = (shmheap_header*)((char*)next_header + next_header->sz + sizeof(shmheap_header));
            next_next_header->prev_sz = (int)header->sz;
            //printf("Next next header initial details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", next_next_header, next_next_header->sz, next_next_header->prev_sz, next_next_header->occupied, next_next_header->last_header);
        }
    }
    if ((size_t)header->prev_sz > 0) {
        size_t prev_sz = (size_t)header->prev_sz;
        shmheap_header* prev_header = (shmheap_header*)((char*)header - prev_sz - sizeof(shmheap_header));
        //printf("Previous header details: address = %p, size= %d, prev_size = %d, occupied = %d, last_header= %d\n", prev_header, prev_header->sz, prev_header->prev_sz, prev_header->occupied, prev_header->last_header);
        if (prev_header->occupied < 1) {
            prev_header->sz = header->sz + prev_header->sz + (int)sizeof(shmheap_header);
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
    //sem_wait(&shmheap_mutex);
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    shmheap_object_handle* objhand = (shmheap_object_handle*)malloc(sizeof(shmheap_object_handle));
    int p = (char*)ptr - (char*)mem.baseaddr;
    objhand->offset = p;
    sem_post(&(hdlptr->shmheap_mutex));
    return *objhand;
}

void* shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl) {
    /* TODO */
    //sem_wait(&shmheap_mutex);
    shmheap_memory_handle* hdlptr = &mem;
    sem_wait(&(hdlptr->shmheap_mutex));
    void* baseaddr = mem.baseaddr;
    int offset = hdl.offset;
    char* target = (char*)baseaddr + offset;
    //sem_post(&shmheap_mutex);
    sem_post(&(hdlptr->shmheap_mutex));
    return target;
}