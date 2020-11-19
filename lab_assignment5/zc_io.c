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

// The zc_file struct is analogous to the FILE struct that you get from fopen.
struct zc_file {
    // Insert the fields you need here.
    //Mutex can have only one reader or writer at a time,
    //RwLock can have one writer or multiple reader at a time.
    pthread_rwlock_t rwlock;
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
    // Allocate the basic metadata structure
    void* front_ptr;
    int fd;
    off_t offset = 0;
    size_t size;
    zc_file* zerocpy = (zc_file*)malloc(sizeof(zc_file));
    struct stat buf;

    //Open the file
    if ((fd = open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) == -1) {
        printf("open file failed\n");
        exit(1);
    }
    //Get size of file for mmap
    if (fstat(fd, &buf) == -1) {
        printf("get stats failed\n");
        exit(1);
    }
    //mmap if file size is not zero
    if (buf.st_size != 0) {
        if ((front_ptr = mmap(NULL, buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
            printf("mmap failed\n");
            exit(1);
        }
    }
    size = buf.st_size;
    pthread_rwlock_init(&(zerocpy->rwlock), NULL);
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
    munmap(file->front_ptr, file->size);
    close(file->fd);
    pthread_rwlock_destroy(&(file->rwlock));
    pthread_mutex_unlock(&(file->mutex));
    free(file);
    pthread_mutex_destroy(&(file->mutex));
    return 0;
}

const char* zc_read_start(zc_file* file, size_t* size) { //point to buffer at least size bytes big
    // To implement
    char* address_ret = NULL;
    //Get reader lock
    pthread_rwlock_rdlock(&(file->rwlock));
    //Get  lock for metadata access
    pthread_mutex_lock(&(file->mutex));

    if (file->offset == file->size) {
        //If file is completely read
        return address_ret;
    }
    if (file->size < (file->offset + (*size))) {
        //When size is greater than the remaining size to be read
        (*size) = (file->size - file->offset); //set size to correct amount that's left
    }

    address_ret = ((char*)(file->front_ptr) + file->offset);

    file->offset += (*size); //increase the offset
    pthread_mutex_unlock(&(file->mutex));

    return address_ret;
}

void zc_read_end(zc_file* file) {
    // To implement
    pthread_rwlock_unlock(&(file->rwlock));
}

/**************
 * Exercise 2 *
 **************/

char* zc_write_start(zc_file* file, size_t size) {
    // To implement
    void* new_addr, * ret_addr = NULL;
    //Get writer lock
    pthread_rwlock_wrlock(&(file->rwlock));
    pthread_mutex_lock(&(file->mutex));


    if (file->size != 0 && (size + file->offset) > file->size) {
        //If file does not have sufficient space
        if ((new_addr = mremap(file->front_ptr, file->size, (file->offset + size), MREMAP_MAYMOVE)) == (void*)-1) {
            printf("mremap failed : %s\n", strerror(errno));
            return ret_addr;
        }
        //Resize to make it grow bigger
        ftruncate(file->fd, size + file->offset);
        file->front_ptr = new_addr;
        file->size = (file->offset + size);
    }

    if (file->size == 0) {
        //If this is new file created, mmap now
        if ((new_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file->fd, 0)) == MAP_FAILED) {
            printf("mmap failed(for newly created file)\n");
            exit(1);
        }
        ftruncate(file->fd, size);
        file->front_ptr = new_addr;
        file->size = (size);
    }

    ret_addr = ((char*)file->front_ptr + file->offset);

    file->offset += size;
    pthread_mutex_unlock(&(file->mutex));

    return ret_addr;
}

void zc_write_end(zc_file* file) {
    // To implement
    // msync is for flushing of the changes to file
    if ((msync(file->front_ptr, file->size, MS_SYNC)) == -1) {
        printf("msync failed with %s\n", strerror(errno));
    }
    pthread_rwlock_unlock(&(file->rwlock));
}

/**************
 * Exercise 3 *
 **************/

off_t zc_lseek(zc_file* file, long offset, int whence) { //repositioning the file offset
    // To implement
    off_t repositioned_offset;
    pthread_rwlock_wrlock(&(file->rwlock));
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
    pthread_rwlock_unlock(&(file->rwlock));
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