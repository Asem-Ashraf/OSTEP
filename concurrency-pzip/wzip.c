#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define INT_SIZE 4
#define CHAR_SIZE 1
#define MIN_CHUNK_SIZE 100
#define MAX_QUEUE_SIZE 300
#define MAX_PRODUCER 200
#define CHUNK_PER_THREAD 3
#define THREAD_FACTOR 1

unsigned int globalProcessedCount = 0;
unsigned int globalChunkCount= 0;
unsigned int globalfilecount = 0;
unsigned int globalcount = 0;
unsigned int allowedChunksAtATime = MAX_PRODUCER; // the maximum number of chunks that can be allocated at the same time.


pthread_mutex_t doneLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t makeChunkLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doneCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t makeChunkCV = PTHREAD_COND_INITIALIZER;

typedef struct 
chunk {
    char*                start;           // start of the chunk uncompressed
    char*                compressed;      // compressed chunk
    unsigned long long   first_count;     // number of times the first character appears
    unsigned long long   last_count;      // number of times the last character appears
    unsigned int         compressed_size; // size of the compressed chunk
    unsigned int         size;            // number of bytes of the chunk
    unsigned int         changes;         // how many times the character changed
    unsigned int         done;            // if the chunk is done compressing
    char                 first;           // first character in the chunk
    char                 last;            // lat character in the chunk
}chunk;

typedef struct 
file_t{
    char*               file;
    chunk*              chunks;
    unsigned long long  chunkcount;
    unsigned long long  size;
    unsigned int        chunksize;
    int                 fd;
}file;

void 
compress_chunk(chunk *c) {
    int count = 1;
    for (int i = 0; i < c->size-1; i++) {
        if (c->start[i] == c->start[i+1]) {
            count++;
        }
        else { // Not the last character but the next character is different.
            c->changes++;// increment the number of character changes. if == 0, then the chunk is all the same character.
            memcpy(c->compressed+c->compressed_size, &count, INT_SIZE);// copy the count of the character
            c->compressed_size += INT_SIZE;// increment the compressed size
            memcpy(c->compressed+c->compressed_size, &c->start[i], CHAR_SIZE);// copy the character
            c->compressed_size += CHAR_SIZE;// increment the compressed size
            count = 1; // reset the count
        }
        if (c->changes == 0){ // if on first character, keep saving the count of the first character
            c->first_count = count;
        }
    }
    // copy the last character
    c->last = c->start[c->size-1];
    c->last_count = count;
    memcpy(c->compressed+c->compressed_size, &c->last_count, INT_SIZE);
    c->compressed_size += INT_SIZE;
    memcpy(c->compressed+c->compressed_size, &c->last, CHAR_SIZE);
    c->compressed_size += CHAR_SIZE;

    // mark the chunk as done
    pthread_mutex_lock(&doneLock);
    c->done = 1;
    pthread_cond_signal(&doneCV); // signal that a chunk is done
    pthread_mutex_unlock(&doneLock);
}

// mutex for producer consumer threads.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty= PTHREAD_COND_INITIALIZER;
pthread_cond_t fill= PTHREAD_COND_INITIALIZER;

// queue and its variables for producer consumer threads.
chunk* queue[MAX_QUEUE_SIZE];
int useptr = 0;
int fillptr = 0;
int numfull = 0;

void AddToQueue(chunk *c) {
    // add chunk to a queue (atomic)
    queue[fillptr] = c;
    fillptr = (fillptr + 1) % MAX_QUEUE_SIZE;
    numfull++;
}

chunk* RemoveFromQueue() {
    // remove chunk from a queue (atomic)
    chunk *c = queue[useptr];
    useptr = (useptr + 1) % MAX_QUEUE_SIZE;
    numfull--;
    return c;
}


void* producer(file* file) {
    for (int i=0; i<file->chunkcount; i++) {
        
        pthread_mutex_lock(&makeChunkLock);
        while(allowedChunksAtATime == 0 ) {
            // wait until a chunk is freed.
            pthread_cond_wait(&makeChunkCV, &makeChunkLock);
        }
        allowedChunksAtATime--;
        pthread_mutex_unlock(&makeChunkLock);

        // initialize each chunk
        if (file->size>=(i+1)*file->chunksize) file->chunks[i].size = file->chunksize; // assign a full sized chunk
        else{ 
            file->chunks[i].size = file->size - (i * file->chunksize); // assign the remaining size to the last chunk 
        }
        file->chunks[i].compressed = (char *)malloc((INT_SIZE+CHAR_SIZE)*file->chunks[i].size); // the maximum size of a compressed chunk
        assert(file->chunks[i].compressed != NULL);
        file->chunks[i].compressed_size = 0;
        file->chunks[i].first =file->file[i*file->chunksize];
        file->chunks[i].first_count = 1;
        file->chunks[i].last_count = 1;
        file->chunks[i].changes = 0;
        file->chunks[i].done= 0;
        file->chunks[i].start = file->file + (i * file->chunksize);

        // push the chunk to the queue.
        pthread_mutex_lock(&mutex);
        while (numfull == MAX_QUEUE_SIZE) {
            // wait for the queue to have space
            pthread_cond_wait(&empty, &mutex);
        }
        // add chunk to a queue
        AddToQueue(&file->chunks[i]);
        pthread_cond_signal(&fill);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void* consumer(void* args) { 
    while (1) {
        pthread_mutex_lock(&mutex);
        while (numfull == 0) {
            if ((globalProcessedCount == globalChunkCount)&&(globalChunkCount!=0)&&(globalfilecount==0)) { // if all chunks from all files are processed
                pthread_mutex_unlock(&mutex);
                goto all_done;
            }
            pthread_cond_wait(&fill, &mutex);
        }
        // remove chunk from a queue
        chunk *c = RemoveFromQueue();
        globalProcessedCount++;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        compress_chunk(c);
    }
all_done:
    pthread_cond_broadcast(&fill);// signal all consumers to exit
    pthread_cond_signal(&doneCV);// signal all to main thread that all chunks are done
    return NULL;
}

void Fwrite(int count, char letter) {
    int rc = fwrite(&count, INT_SIZE, 1, stdout);
    assert(rc == 1);
    rc = fwrite(&letter, CHAR_SIZE, 1, stdout);
    assert(rc == 1);
}


int
main(int argc, char *argv[]){
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }
    // NO. of cores
    int cores = get_nprocs();
    int threadcount = cores/THREAD_FACTOR;

    // Array of producer threads
    pthread_t pid[argc-1];
    globalfilecount = argc-1;

    // Array of consumer threads
    pthread_t cid[threadcount];
    for (int i=0; i<threadcount; i++) {
        int* id = (int *)malloc(sizeof(int));
        *id = i;
        int rc = pthread_create(&cid[i], NULL, (void*(*)(void *))consumer, id);
        assert(rc == 0);
    }


    file files[argc-1];
    struct stat st;
    for (int i=0; i<argc-1; i++) {
        files[i].fd  = open(argv[i+1], O_RDONLY);
        assert(files[i].fd != -1);
        fstat(files[i].fd, &st);
        files[i].size = st.st_size;
        files[i].file = (char *)
            mmap(NULL,  // the location of the file in the contiguous memory block
                 files[i].size,// the size of the file
                 PROT_READ,  // read only
                 MAP_SHARED, // private
                 files[i].fd,// the file descriptor
                 0);         // the offset of the file.
        assert(files[i].file != MAP_FAILED);

        // Calculate the chunk size and chunk count
        if (files[i].size/ (MIN_CHUNK_SIZE) <= 1) {
            files[i].chunkcount = 1;
            files[i].chunksize = files[i].size;
        }
        else{
            if (files[i].size% (MIN_CHUNK_SIZE)>0 ) {
                files[i].chunkcount = files[i].size/ (MIN_CHUNK_SIZE)+1;
            }
            else {
                files[i].chunkcount = files[i].size/ (MIN_CHUNK_SIZE);
            }
            files[i].chunksize = MIN_CHUNK_SIZE;
        }

        // Array of chunks
        files[i].chunks = (chunk *)malloc(sizeof(chunk)*files[i].chunkcount);
        assert(files[i].chunks != NULL);
        globalChunkCount+=files[i].chunkcount;
        globalfilecount--;
        int rc = pthread_create(&pid[i], NULL, (void*)producer, &files[i]);
        assert(rc == 0);
    }
    // writing all chunks to stdout
    for (int i = 0 ; i<argc-1; i++){ // for every file
        for (unsigned long long k=0; k<files[i].chunkcount; k++) { // for every chunk in that file
            pthread_mutex_lock(&doneLock);
            while(files[i].chunks[k].done == 0 ) { // wait for the chunk to be done
                pthread_cond_wait(&doneCV, &doneLock);
            }
            pthread_mutex_unlock(&doneLock);
            if (files[i].chunks[k].changes== 0) { // if no changes in the chunk, unit the first and last character counts
                files[i].chunks[k].last_count= files[i].chunks[k].first_count;
            }

            for (int l=0; l<files[i].chunks[k].compressed_size-(INT_SIZE+CHAR_SIZE);l+=5) { // print the compressed chunk except the last count
                Fwrite(*(int*)(files[i].chunks[k].compressed+l),                            // if there is no change, nothing will be printed.
                       *(char*)(files[i].chunks[k].compressed+INT_SIZE+l));
            }
            if(k<files[i].chunkcount-1){                                    // if not the last chunk in the file
                pthread_mutex_lock(&doneLock);
                while(files[i].chunks[k+1].done == 0 ) { // wait for the next chunk to be done
                    pthread_cond_wait(&doneCV, &doneLock);
                }
                pthread_mutex_unlock(&doneLock);
                if (files[i].chunks[k+1].first==files[i].chunks[k].last) { // if the first letter of the next chunk is the same as the last letter of the current chunk
                    files[i].chunks[k+1].first_count += files[i].chunks[k].last_count; 
                    memcpy(files[i].chunks[k+1].compressed, &files[i].chunks[k+1].first_count, INT_SIZE);
                }
                else {
                    Fwrite(files[i].chunks[k].last_count, files[i].chunks[k].last); 
                }
            }
            else {                                                          //if last chunk
                if (i<argc-1-1) {                                           // if not last file
                    pthread_mutex_lock(&doneLock);
                    while(files[i+1].chunks[0].done == 0 ) {
                        // wait for the first chunk of the next file to be done
                        pthread_cond_wait(&doneCV, &doneLock);
                    }
                    pthread_mutex_unlock(&doneLock);
                    if (files[i+1].chunks[0].first==files[i].chunks[k].last) { // if the first letter of the first chunk of the next file is the same as the last letter of the current chunk
                        files[i+1].chunks[0].first_count += files[i].chunks[k].last_count; 
                        memcpy(files[i+1].chunks[0].compressed, &files[i+1].chunks[0].first_count, INT_SIZE);
                    }
                    else {
                        Fwrite(files[i].chunks[k].last_count, files[i].chunks[k].last); 
                    }
                }
                else {
                    Fwrite(files[i].chunks[k].last_count, files[i].chunks[k].last); 
                }
            }
            free(files[i].chunks[k].compressed); // free the compressed chunk
            pthread_mutex_lock(&makeChunkLock);
            // signal the producer that the chunk is freed and can be used again
            allowedChunksAtATime++;
            pthread_cond_signal(&makeChunkCV);
            pthread_mutex_unlock(&makeChunkLock);
        }
        free(files[i].chunks);
        munmap(files[i].file, files[i].size);
        close(files[i].fd);
    }
}
