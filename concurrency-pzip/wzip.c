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
#define STANDARD_CHUNK_SIZE 100
#define MAX_QUEUE_SIZE 300
#define MAX_ALLOCATED_CHUNKS 200
#define CHUNK_PER_THREAD 3
#define THREAD_FACTOR 1

unsigned int globalProcessedCount = 0;
unsigned int globalChunkCount= 0;
unsigned int globalfilecount = 0;
unsigned int globalcount = 0;
// the maximum number of chunks that can be allocated at the same time. This is
// to constrain the memory usage. If more memory for chunks is available, this 
// number can be increased.
unsigned int AvailableChunkSpaces = MAX_ALLOCATED_CHUNKS;


pthread_mutex_t doneLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  doneCV   = PTHREAD_COND_INITIALIZER;

pthread_mutex_t makeChunkLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  makeChunkCV   = PTHREAD_COND_INITIALIZER;

typedef struct 
chunk {
    char*                chunkBegining;           // start of the chunk uncompressed
    unsigned int         chunkSize;               // number of bytes of the chunk because chunks might not all be equal in size
    char*                compressedChunkBegining; // compressed chunk
    unsigned long long   firstCharCount;          // number of times the first character appears
    unsigned long long   lastCharCount;           // number of times the last character appears
    unsigned int         resultantCompressedSize; // size of the compressed chunk
    unsigned int         differentCharInChunk;    // how many times the character changed
    unsigned int         compressingDone;         // if the chunk is done compressing
    char                 firstCharInChunk;        // first character in the chunk
    char                 lastCharInChunk;         // lat character in the chunk
}chunk;

typedef struct 
myfile_t{
    char*               fileCharArray;
    chunk*              chunksArray;
    unsigned long long  chunkCount;
    unsigned long long  fileSize;
    unsigned int        biggestChunkSize;
    int                 fd;
}myFile_t;

void 
compress_chunk(chunk *c) {
    int count = 1;
    for (int i = 0; i < c->chunkSize-1; i++) {
        if (c->chunkBegining[i] == c->chunkBegining[i+1]) {
            count++;
        }
        else { // Not the last character but the next character is different.
            c->differentCharInChunk++;// increment the number of character changes. if == 0, then the chunk is all the same character.
            memcpy(c->compressedChunkBegining+c->resultantCompressedSize, &count, INT_SIZE);// copy the count of the character
            c->resultantCompressedSize += INT_SIZE;// increment the compressed size
            memcpy(c->compressedChunkBegining+c->resultantCompressedSize, &c->chunkBegining[i], CHAR_SIZE);// copy the character
            c->resultantCompressedSize += CHAR_SIZE;// increment the compressed size
            count = 1; // reset the count
        }
        if (c->differentCharInChunk == 0){ // if on first character, keep saving the count of the first character
            c->firstCharCount = count;
        }
    }
    // copy the last character
    c->lastCharInChunk = c->chunkBegining[c->chunkSize-1];
    c->lastCharCount = count;
    memcpy(c->compressedChunkBegining+c->resultantCompressedSize, &c->lastCharCount, INT_SIZE);
    c->resultantCompressedSize += INT_SIZE;
    memcpy(c->compressedChunkBegining+c->resultantCompressedSize, &c->lastCharInChunk, CHAR_SIZE);
    c->resultantCompressedSize += CHAR_SIZE;

    // mark the chunk as done
    pthread_mutex_lock(&doneLock);
    c->compressingDone = 1;
    pthread_cond_signal(&doneCV); // signal that a chunk is done
    pthread_mutex_unlock(&doneLock);
}

// mutex for producer consumer threads.
pthread_mutex_t PCQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t PCQueueSpaceAvailableCV= PTHREAD_COND_INITIALIZER;
pthread_cond_t PCQueueNotEmptyCV= PTHREAD_COND_INITIALIZER;

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


void* producer(myFile_t* file) {
    for (int i=0; i<file->chunkCount; i++) {
        
        pthread_mutex_lock(&makeChunkLock);
        while(AvailableChunkSpaces == 0 ) {
            // wait until a chunk is freed.
            pthread_cond_wait(&makeChunkCV, &makeChunkLock);
        }
        AvailableChunkSpaces--;
        pthread_mutex_unlock(&makeChunkLock);

        // initialize each chunk


        // init chunk size
        if (file->fileSize >= (i+1)*file->biggestChunkSize)
            // assign a full sized chunk
            file->chunksArray[i].chunkSize = file->biggestChunkSize;
        else{ 
            // if the last chunk is smaller than the standard chunk size
            // assign the remaining size to the last chunk 
            file->chunksArray[i].chunkSize = file->fileSize - (i * file->biggestChunkSize);
        }


        // dynamically allocate memory for the chunk
        file->chunksArray[i].compressedChunkBegining = (char *)malloc((INT_SIZE+CHAR_SIZE)*file->chunksArray[i].chunkSize); // the maximum size of a compressed chunk
        assert(file->chunksArray[i].compressedChunkBegining != NULL);

        // init the rest of the chunk
        file->chunksArray[i].resultantCompressedSize = 0;
        file->chunksArray[i].firstCharInChunk =file->fileCharArray[i*file->biggestChunkSize];
        file->chunksArray[i].firstCharCount = 1;
        file->chunksArray[i].lastCharCount = 1;
        file->chunksArray[i].differentCharInChunk = 0;
        file->chunksArray[i].compressingDone= 0;
        file->chunksArray[i].chunkBegining = file->fileCharArray + (i * file->biggestChunkSize);

        // push the chunk to the queue.
        pthread_mutex_lock(&PCQueueLock);
        while (numfull == MAX_QUEUE_SIZE) {
            // wait for the queue to have space
            pthread_cond_wait(&PCQueueSpaceAvailableCV, &PCQueueLock);
        }
        // add chunk to a queue
        AddToQueue(&file->chunksArray[i]);
        pthread_cond_signal(&PCQueueNotEmptyCV);
        pthread_mutex_unlock(&PCQueueLock);
    }
    return NULL;
}

void* consumer(void* args) { 
    while (1) {
        pthread_mutex_lock(&PCQueueLock);
        while (numfull == 0) {
            // if all chunks from all files are processed
            if ((globalProcessedCount == globalChunkCount)&&(globalChunkCount!=0)&&(globalfilecount==0)) {
                pthread_mutex_unlock(&PCQueueLock);
                // break from nested while loops
                goto all_done;
            }
            pthread_cond_wait(&PCQueueNotEmptyCV, &PCQueueLock);
        }
        // remove chunk from a queue
        chunk *c = RemoveFromQueue();
        globalProcessedCount++;
        pthread_cond_signal(&PCQueueSpaceAvailableCV);
        pthread_mutex_unlock(&PCQueueLock);
        compress_chunk(c);
    }
all_done:
    // wake all consumers that are waiting for the queue to be not empty
    pthread_cond_broadcast(&PCQueueNotEmptyCV);
    // signal to main that all chunks are processed
    pthread_cond_signal(&doneCV);
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
    // for each file there will be a producer.
    pthread_t pid[argc-1];

    // a global file count to be used by (other functions)
    globalfilecount = argc-1;

    // Array of consumer threads that equals the count of cores/THREAD_FACTOR
    pthread_t cid[threadcount];
    for (int i=0; i<threadcount; i++) {
        // Starting each thread and passing the thread its unique id as an 
        // argument
        // dynamically allocating the id variable so it persists even outside 
        // this scope
        int* id = (int *)malloc(sizeof(int));
        *id = i;
        int rc = pthread_create(&cid[i], NULL, (void*(*)(void *))consumer, id);
        assert(rc == 0);
    }


    // array of a file struct that I defined passed as arguments
    myFile_t files[argc-1];
    struct stat st;
    for (int i=0; i<argc-1; i++) {
        // open a file from the arguments
        files[i].fd  = open(argv[i+1], O_RDONLY);
        assert(files[i].fd != -1);
        // get details about that file
        fstat(files[i].fd, &st);

        files[i].fileSize = st.st_size;
        files[i].fileCharArray = (char *)
            mmap(NULL,          // the location of the file in the contiguous memory block
                 files[i].fileSize, // the size of the file
                 PROT_READ,     // read only
                 MAP_SHARED,    // shared with other processes
                 files[i].fd,   // the file descriptor
                 0);            // the offset of the file.
        assert(files[i].fileCharArray != MAP_FAILED);

        // Calculate the chunk size and chunk count
        // if the file size is less than the minimum chunk size
        if (files[i].fileSize/ (STANDARD_CHUNK_SIZE) <= 1) {
            // assign the chunk count to 1
            files[i].chunkCount = 1;
            // assign the size of each chunk in this file to the size of the file
            // itself since the file size is less than the standard chunk size
            // and there is only one chunk for this file
            files[i].biggestChunkSize = files[i].fileSize;
        }
        else{
            // if the file size is not divisible by the standard chunk size
            if (files[i].fileSize% (STANDARD_CHUNK_SIZE)>0 ) {
                // then the last chuck in for this file is less than the size of 
                // the minimum chunk
                files[i].chunkCount = files[i].fileSize/ (STANDARD_CHUNK_SIZE)+1;
            }
            else {
                // else the last chuck in for this file is equal to the size of
                // the minimum chunk
                files[i].chunkCount = files[i].fileSize/ (STANDARD_CHUNK_SIZE);
            }
            // assign the chunk size to the standard chunk size
            files[i].biggestChunkSize = STANDARD_CHUNK_SIZE;
        }

        // Array of chunks for this file
        files[i].chunksArray = (chunk *)malloc(sizeof(chunk)*files[i].chunkCount);
        assert(files[i].chunksArray != NULL);

        // broadcast that there are new chunks to be processed
        globalChunkCount+=files[i].chunkCount;
        globalfilecount--;

        // start a producer thread for this file
        int rc = pthread_create(&pid[i], NULL, (void*)producer, &files[i]);
        assert(rc == 0);
    }




    // in this for loop, we wait for each chunk in order to print it
    
    
    
    // for every file
    for (int i = 0 ; i<argc-1; i++){ 
        // for every chunk in that file
        for (unsigned long long k=0; k<files[i].chunkCount; k++) { 
            pthread_mutex_lock(&doneLock);
            // wait for the chunk to be done
            while(files[i].chunksArray[k].compressingDone == 0 ) { 
                pthread_cond_wait(&doneCV, &doneLock);
            }
            pthread_mutex_unlock(&doneLock);
            // if no changes in the chunk, unit the first and last character counts
            if (files[i].chunksArray[k].differentCharInChunk== 0) {
                files[i].chunksArray[k].lastCharCount= files[i].chunksArray[k].firstCharCount;
            }

            for (int l=0; l<files[i].chunksArray[k].resultantCompressedSize-(INT_SIZE+CHAR_SIZE);l+=5) { // print the compressed chunk except the last count
                Fwrite(*(int*)(files[i].chunksArray[k].compressedChunkBegining+l),                            // if there is no change, nothing will be printed.
                       *(char*)(files[i].chunksArray[k].compressedChunkBegining+INT_SIZE+l));
            }
            if(k<files[i].chunkCount-1){                                    // if not the last chunk in the file
                pthread_mutex_lock(&doneLock);
                while(files[i].chunksArray[k+1].compressingDone == 0 ) { // wait for the next chunk to be done
                    pthread_cond_wait(&doneCV, &doneLock);
                }
                pthread_mutex_unlock(&doneLock);
                if (files[i].chunksArray[k+1].firstCharInChunk==files[i].chunksArray[k].lastCharInChunk) { // if the first letter of the next chunk is the same as the last letter of the current chunk
                    files[i].chunksArray[k+1].firstCharCount += files[i].chunksArray[k].lastCharCount; 
                    memcpy(files[i].chunksArray[k+1].compressedChunkBegining, &files[i].chunksArray[k+1].firstCharCount, INT_SIZE);
                }
                else {
                    Fwrite(files[i].chunksArray[k].lastCharCount, files[i].chunksArray[k].lastCharInChunk); 
                }
            }
            else {                                                          //if last chunk
                if (i<argc-1-1) {                                           // if not last file
                    pthread_mutex_lock(&doneLock);
                    while(files[i+1].chunksArray[0].compressingDone == 0 ) {
                        // wait for the first chunk of the next file to be done
                        pthread_cond_wait(&doneCV, &doneLock);
                    }
                    pthread_mutex_unlock(&doneLock);
                    if (files[i+1].chunksArray[0].firstCharInChunk==files[i].chunksArray[k].lastCharInChunk) { // if the first letter of the first chunk of the next file is the same as the last letter of the current chunk
                        files[i+1].chunksArray[0].firstCharCount += files[i].chunksArray[k].lastCharCount; 
                        memcpy(files[i+1].chunksArray[0].compressedChunkBegining, &files[i+1].chunksArray[0].firstCharCount, INT_SIZE);
                    }
                    else {
                        Fwrite(files[i].chunksArray[k].lastCharCount, files[i].chunksArray[k].lastCharInChunk); 
                    }
                }
                else {
                    Fwrite(files[i].chunksArray[k].lastCharCount, files[i].chunksArray[k].lastCharInChunk); 
                }
            }
            free(files[i].chunksArray[k].compressedChunkBegining); // free the compressed chunk
            pthread_mutex_lock(&makeChunkLock);
            // signal the producer that the chunk is freed and can be used again
            AvailableChunkSpaces++;
            pthread_cond_signal(&makeChunkCV);
            pthread_mutex_unlock(&makeChunkLock);
        }
        free(files[i].chunksArray);
        munmap(files[i].fileCharArray, files[i].fileSize);
        close(files[i].fd);
    }
}
