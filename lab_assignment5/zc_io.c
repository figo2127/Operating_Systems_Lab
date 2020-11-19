#include "zc_io.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

// The zc_file struct is analogous to the FILE struct that you get from fopen.
struct zc_file {
    // Insert the fields you need here.
    //Mutex can have only one reader or writer at a time,
    //RwLock can have one writer or multiple reader at a time.
    pthread_rwlock_t lock_for_rw;
    pthread_mutex_t mutex;
    void* front_ptr;
    int fd;
    off_t offset;
    size_t size;
};

/**************
 * Exercise 1 *
 **************/

zc_file* zc_open(const char* path) {
    // To implement
    void* front_ptr;
    int fd;
    off_t offset = 0;
    size_t size;
    zc_file* zerocpy = (zc_file*)malloc(sizeof(zc_file));
    struct stat buf;

    if ((fd = open(path, O_CREAT | O_RDWR, S_IRWXG | S_IRWXO | S_IRWXU)) == -1) {
        printf("open file failed\n");
        exit(1);
    }
    if (fstat(fd, &buf) == -1) {
        printf("get stats failed\n");
        exit(1);
    }
    if (buf.st_size != 0) {
        if ((front_ptr = mmap(NULL, buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED_VALIDATE, fd, 0)) == MAP_FAILED) {
            printf("mmap failed\n");
            exit(1);
        }
    }
    size = buf.st_size;
    pthread_rwlock_init(&(zerocpy->lock_for_rw), NULL);
    pthread_mutex_init(&(zerocpy->mutex), NULL);
    zerocpy->front_ptr = front_ptr;
    zerocpy->fd = fd;
    zerocpy->offset = offset;
    zerocpy->size = size;
    return zerocpy;
}

int zc_close(zc_file* file) {
    // To implement
    pthread_mutex_lock(&(file->mutex));
    //unmap the mapped file, indicating its size
    munmap(file->front_ptr, file->size);
    close(file->fd);
    pthread_rwlock_destroy(&(file->lock_for_rw));
    pthread_mutex_unlock(&(file->mutex));
    free(file);
    pthread_mutex_destroy(&(file->mutex));
    return 0;
}

const char* zc_read_start(zc_file* file, size_t* size) { //returns pointer to buffer at least 'size' bytes 
    // To implement
    char* ptr = NULL;
    size_t totalsize = file->size;
    pthread_rwlock_rdlock(&(file->lock_for_rw));
    pthread_mutex_lock(&(file->mutex));

    if (totalsize == (size_t)file->offset) { //nothing left to read
        return ptr;
    }
    if (totalsize < (file->offset + (*size))) {
        //assign size to the correct available amount
        (*size) = (totalsize - file->offset); 
    }
    ptr = ((char*)(file->front_ptr) + file->offset);
    file->offset = file->offset + (*size); //increase the offset
    pthread_mutex_unlock(&(file->mutex));
    return ptr;
}

void zc_read_end(zc_file* file) {
    // To implement
    pthread_rwlock_unlock(&(file->lock_for_rw));
}

/**************
 * Exercise 2 *
 **************/

char* zc_write_start(zc_file* file, size_t size) { //return ptr to buffer of at least 'size' bytes that can be written
    // To implement
    void* new_ptr = NULL;
    void* ret_ptr = NULL;
    pthread_rwlock_wrlock(&(file->lock_for_rw));
    pthread_mutex_lock(&(file->mutex));

    size_t totalsize = file->size;
    if (totalsize == 0) { //handle new file, create a memory mapping 
        if ((new_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file->fd, 0)) == MAP_FAILED) {
            printf("mmap failed(for newly created file)\n");
            exit(1);
        }
        ftruncate(file->fd, size);
        file->size = size;
        file->front_ptr = new_ptr;
    }
    //void* mremap(void* old_address, size_t old_size,
    //    size_t new_size, int flags, ... /* void *new_address */);
    if ((size + file->offset) > totalsize) {
        //If file does not have sufficient space
        size_t new_size = size + file->offset;
        if ((new_ptr = mremap(file->front_ptr, totalsize, new_size, MREMAP_MAYMOVE)) == MAP_FAILED) { //expands the memory mapping here
            printf("mremap failed\n");
            exit(1);
        }
        ftruncate(file->fd, new_size); //truncate file to specific length(increase it here)
        file->size = new_size;
        file->front_ptr = new_ptr;
    }

    ret_ptr = ((char*)file->front_ptr + file->offset);
    file->offset += size;
    pthread_mutex_unlock(&(file->mutex));

    return ret_ptr;
}

void zc_write_end(zc_file* file) {
    // To implement
    // msync is for flushing of the changes to file
    if ((msync(file->front_ptr, file->size, MS_SYNC)) == -1) {
        printf("msync failed with %s\n", strerror(errno));
    }
    pthread_rwlock_unlock(&(file->lock_for_rw));
}

/**************
 * Exercise 3 *
 **************/

off_t zc_lseek(zc_file* file, long offset, int whence) { //repositioning the file offset
    // To implement
    off_t repositioned_offset;
    pthread_rwlock_wrlock(&(file->lock_for_rw));
    pthread_mutex_lock(&(file->mutex));
    if (whence == SEEK_SET) {
        repositioned_offset = offset;
    }
    else if (whence == SEEK_CUR) {
        repositioned_offset = (file->offset) + offset;
    }
    else if (whence == SEEK_END) { //SEEK_END: end + offset
        repositioned_offset = (file->size) + offset;
    }
    else {
        perror("invalid whence");
        exit(1);
    }
    file->offset = repositioned_offset;
    pthread_mutex_unlock(&(file->mutex));
    pthread_rwlock_unlock(&(file->lock_for_rw));
    return repositioned_offset;
}

/**************
 * Exercise 5 *
 **************/

int zc_copyfile(const char* source, const char* dest) {
    // To implement
    zc_file* sourceFile = zc_open(source);
    zc_file* destFile = zc_open(dest);
    off_t sourceOffset = 0;
    off_t destOffset = 0;

    zc_read_start(sourceFile, &(sourceFile->size));
    zc_write_start(destFile, sourceFile->size);

    copy_file_range(sourceFile->fd, &(sourceOffset), destFile->fd, &(destOffset), sourceFile->size, 0);

    zc_read_end(sourceFile);
    zc_write_end(destFile);
    zc_close(sourceFile);
    zc_close(destFile);

    return 0;
}