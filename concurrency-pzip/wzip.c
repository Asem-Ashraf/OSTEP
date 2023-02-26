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
#define MIN_CHUNK_SIZE 550
#define MAX_QUEUE_SIZE 30
#define MAX_PRODUCER 20
#define CHUNK_PER_THREAD 3
#define THREAD_FACTOR 1
#define MULTIPLIER 10

unsigned int globalProcessedCount = 0;
unsigned int globalChunkCount= 0;
unsigned int globalfilecount = 0;
unsigned int globalcount = 0;

unsigned int makemorechunks = MAX_PRODUCER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

typedef struct 
chunk {
    char *start;           // start of the chunk uncompressed
    char *compressed;      // compressed chunk
    unsigned long long compressed_size; // size of the compressed chunk
    unsigned long long   size;            // number of bytes of the chunk
    unsigned long long   first_count;     // number of times the first character appears
    unsigned long long   last_count;      // number of times the last character appears
    unsigned long long   changes;         // how many times the character changed
    unsigned long long   done;
    char  first;           // first character in the chunk
    char  last;            // last character in the chunk
}chunk;

typedef struct 
file_t{
    char* file;
    chunk* chunks;
    unsigned long long size;
    unsigned int   fd;
    unsigned long long   chunksize;
    unsigned long long   chunkcount;
}file;

pthread_mutex_t chmutex = PTHREAD_MUTEX_INITIALIZER;

void 
compress_chunk(chunk *c) {
    int count = 1;
    for (int i = 0; i < c->size-1; i++) {
        if (c->start[i] == c->start[i+1]) {
            count++;
        }
        else { // Not the last character but the next character is different.
            c->changes++;
            memcpy(c->compressed+c->compressed_size, &count, INT_SIZE);
            c->compressed_size += INT_SIZE;
            memcpy(c->compressed+c->compressed_size, &c->start[i], CHAR_SIZE);
            c->compressed_size += CHAR_SIZE;
            count = 1; // reset the count
        }
        if (c->changes == 0){ // if on first character, keep saving the count of the first character
            pthread_mutex_lock(&chmutex);
            c->first_count = count;
            pthread_mutex_unlock(&chmutex);
        }
    }
    memcpy(c->compressed+c->compressed_size, &count, INT_SIZE);
    c->compressed_size += INT_SIZE;
    memcpy(c->compressed+c->compressed_size, &c->start[c->size-1], CHAR_SIZE);
    c->compressed_size += CHAR_SIZE;
    c->last = c->start[c->size-1];
    c->last_count = count;
    // pthread_mutex_lock(&mutex1);
    // c->done = 1;
    // pthread_cond_signal(&cond1);
    // pthread_mutex_unlock(&mutex1);
    // printf("                                                    signal compress\n");
}

// mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty= PTHREAD_COND_INITIALIZER;
pthread_cond_t fill= PTHREAD_COND_INITIALIZER;


chunk* queue[MAX_QUEUE_SIZE];
int useptr = 0;
int fillptr = 0;
int numfull = 0;

void AddToQueue(chunk *c) {
    // add chunk to a queue
    queue[fillptr] = c;
    fillptr = (fillptr + 1) % MAX_QUEUE_SIZE;
    numfull++;
}

chunk* RemoveFromQueue() {
    chunk *c = queue[useptr];
    useptr = (useptr + 1) % MAX_QUEUE_SIZE;
    numfull--;
    return c;
}

void* producer(file* file) {
    // printf("running producer\n");
    for (int i=0; i<file->chunkcount; i++) {
        // pthread_mutex_lock(&mutex2);
        // while(makemorechunks == 0 ) {
        //     // printf("producer waiting %d\n", makemorechunks);
        //     pthread_cond_wait(&cond2, &mutex2);
        //     // printf("producer waking %d\n", makemorechunks);
        // }
        // makemorechunks--;
        // pthread_mutex_unlock(&mutex2);
        // initialize each chunk
        if (file->size>=(i+1)*file->chunksize) file->chunks[i].size = file->chunksize; // assign a full sized chunk
        else{ 
            file->chunks[i].size = file->size - (i * file->chunksize); // assign the last chunk the remaining size
        }
        // printf("\n%lld\n",file->chunks[i].size);
        file->chunks[i].compressed = (char *)malloc((INT_SIZE+CHAR_SIZE)*file->chunks[i].size); // the maximum size of the compressed chunk
        assert(file->chunks[i].compressed != NULL);
        file->chunks[i].compressed_size = 0;
        file->chunks[i].first =file->file[i*file->chunksize];
        file->chunks[i].first_count = 0;
        file->chunks[i].last_count = 0;
        file->chunks[i].changes = 0;
        file->chunks[i].done= 0;
        file->chunks[i].start = file->file + (i * file->chunksize);

        // push the chunk to the queue.
        pthread_mutex_lock(&mutex);
        while (numfull == MAX_QUEUE_SIZE) {
            pthread_cond_wait(&empty, &mutex);
        }
        // add chunk to a queue
        // printf("add ck %d at %d\n", i,fillptr);
        AddToQueue(&file->chunks[i]);
        pthread_cond_signal(&fill);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void* consumer(int* thread_id) { 
    // int order;
    // printf("c%d\n",*thread_id);
    while (1) {
        pthread_mutex_lock(&mutex);
        while (numfull == 0) {
            if ((globalProcessedCount == globalChunkCount)&&(globalChunkCount!=0)&&(globalfilecount==0)) {
                pthread_mutex_unlock(&mutex);
                // printf("c%d: all ck done2\n",*thread_id);
                goto all_done;
            }
            // printf("c%d sleep\n",*thread_id);
            pthread_cond_wait(&fill, &mutex);
            // printf("c%d wake\n",*thread_id);
        }
        // remove chunk from a queue
        chunk *c = RemoveFromQueue();
        // order = globalProcessedCount;
        // printf("                c%d rm ck %d, %d\n",*thread_id,order, globalProcessedCount);
        globalProcessedCount++;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        compress_chunk(c);
        // printf("                            c%d com ck %d, pc %d\n",*thread_id, order, globalProcessedCount);
    }
all_done:
    pthread_cond_broadcast(&fill);
    pthread_cond_signal(&cond1);
    // printf("c%d broadcast\n",*thread_id);
    free (thread_id);
    return NULL;
}

void Fwrite(int count, char letter) {
    int rc = fwrite(&count, INT_SIZE, 1, stdout);
    assert(rc == 1);
    rc = fwrite(&letter, CHAR_SIZE, 1, stdout);
    assert(rc == 1);
    // printf("\n%d%c\n", count, letter);
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
        fstat(files[i].fd, &st);
        files[i].size = st.st_size;
        // printf("open file %s fd %d size %lld\n", argv[i+1], files[i].fd,files[i].size);
        files[i].file = (char *)
            mmap(NULL,  // the location of the file in the contiguous memory block
                 files[i].size,// the size of the file
                 PROT_READ,  // read only
                 MAP_SHARED, // private
                 files[i].fd,// the file descriptor
                 0);         // the offset of the file.
        assert(files[i].file != MAP_FAILED);
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

            // Determining the count of chunks based on the number of threads and how many chunks that each thread should process.
            // limiting the count of the chunk per file based on the thread count
            // this avoids making alot of small chunks which might break malloc.

            // int maxChunkSize = (files[i].size/threadcount) + 1;
            // int chunkdivisionfactor= maxChunkSize/MIN_CHUNK_SIZE;
            // if (chunkdivisionfactor<1) {
            //     // then one thread
            //     files[i].chunkcount = 1;
            //     files[i].chunksize = files[i].size;
            // }
            // else {
            //     // multiple threads
            //     files[i].chunkcount = threadcount*CHUNK_PER_THREAD;
            //     files[i].chunksize = (files[i].size/files[i].chunkcount) + 1;
            // }

            // Determining the count of the chunks based on the minimum chunk size and the number of threads.
            // limiting the max chunk size per file based on the thread count.
            // this makes alot of small chunks which increases parallelism.

            // int maxChunkCount = files[i].size/ (MIN_CHUNK_SIZE);
            // if (maxChunkCount/threadcount > 1) { // if the chunkcount is double the threadcount or greater
            //     files[i].chunkcount = maxChunkCount/threadcount;
            //     files[i].chunksize = (files[i].size/files[i].chunkcount) + 1;
            // }
            // else {
            //     files[i].chunkcount = maxChunkCount;
            //     files[i].chunksize = (files[i].size/files[i].chunkcount) + 1;
            // }

        }
        // printf("chunkcount: %lld, chunksize: %lld\n", files[i].chunkcount, files[i].chunksize);

        // Array of chunks
        files[i].chunks = (chunk *)malloc(sizeof(chunk)*files[i].chunkcount);
        assert(files[i].chunks != NULL);
        globalChunkCount+=files[i].chunkcount;
        globalfilecount--;
        // printf("%s is at %p comp %p\n", argv[i], file, uncompressedFiles);
        // printf("%s\n", files[i].file);
        int rc = pthread_create(&pid[i], NULL, (void*)producer, &files[i]);
        assert(rc == 0);
    }
    // printf("waiting\n");

    // wait for producers
    for (int i=0; i<argc-1; i++) {
        int rc = pthread_join(pid[i], NULL);
        assert(rc == 0);
    }
    // printf("waiting\n");
    // wait for consumers
    for (int i=0; i<threadcount; i++) {
        int rc = pthread_join(cid[i], NULL);
        assert(rc == 0);
    }
    // printf("waiting\n");

    for (int i = 0 ; i<argc-1; i++){ // for every file
        for (int k=0; k<files[i].chunkcount; k++) {                                       // for every chunk in that file
            // pthread_mutex_lock(&mutex1);
            // while(files[i].chunks[k].done == 0 ) {
            //     // printf("file %d chunk %d wait\n", i ,k);
            //     pthread_cond_wait(&cond1, &mutex1);
            //     // printf("file %d chunk %d        wake\n", i ,k);
            // }
            // pthread_mutex_unlock(&mutex1);
            // printf("\nfile %d chunk %d done\n", i ,k);
            // printf("\nchanges\n");
            // printf("compressed chunk size %d, address %p\n", files[i].chunks[k].compressed_size, files[i].chunks[k].compressed);
            if (files[i].chunks[k].changes== 0) {
                files[i].chunks[k].last_count= files[i].chunks[k].first_count;

            }
            for (long long l=0; l<files[i].chunks[k].compressed_size-(INT_SIZE+CHAR_SIZE);l+=5) { // print the compressed chunk except the last count
                // printf("\nwriting %d %c\n", *(int*)(files[i].chunks[k].compressed+l), *(char*)(files[i].chunks[k].compressed+INT_SIZE+l));
                Fwrite(*(int*)(files[i].chunks[k].compressed+l),
                       *(char*)(files[i].chunks[k].compressed+INT_SIZE+l));
                // printf("writing from %lld %lld\n", (files[i].chunks[k].compressed+l), (files[i].chunks[k].compressed+INT_SIZE+l));
            }
            if (i<argc-1-1) {                                                   // changes in the last chunk but not the last file
                if(k<files[i].chunkcount-1){                                    // no changes, not the last file but not the last chunk in the file
                    if (files[i].chunks[k+1].first==files[i].chunks[k].last) { // if the next chunk in the file has the same first letter
                        // pthread_mutex_lock(&chmutex);
                        files[i].chunks[k+1].first_count += files[i].chunks[k].last_count; 
                        // pthread_mutex_unlock(&chmutex);
                    }
                    else {                                                      // if the next chunk in the file has a different first letter
                        Fwrite(files[i].chunks[k].last_count, files[i].chunks[k].last); 
                    }
                }
                else {                                                          // if not last file but last chunk
                    // pthread_mutex_lock(&mutex1);
                    // while(files[i+1].chunks[0].done == 0 ) {
                    //     // printf("file %d chunk %d wait\n", i ,k);
                    //     pthread_cond_wait(&cond1, &mutex1);
                    //     // printf("file %d chunk %d        wake\n", i ,k);
                    // }
                    // pthread_mutex_unlock(&mutex1);
                    if (files[i+1].chunks[0].first==files[i].chunks[k].last) { // if the first chunk in the next file has the same first letter

                        // pthread_mutex_lock(&chmutex);
                        files[i+1].chunks[0].first_count += files[i].chunks[k].last_count; 
                        // pthread_mutex_unlock(&chmutex);
                    }
                    else {                                                      // if the first chunk in the next file has a different first letter
                    Fwrite(files[i].chunks[k].last_count, files[i].chunks[k].last); 
                }
                }
            }
            else {                                                              // if no changes and last file
            if(k<files[i].chunkcount-1){                                    // no changes but not the last chunk in the last file
                if (files[i].chunks[k+1].first==files[i].chunks[k].last) { 
                    // pthread_mutex_lock(&chmutex);
                    files[i].chunks[k+1].first_count += files[i].chunks[k].last_count; 
                    // pthread_mutex_unlock(&chmutex);
                }
                else {
                    Fwrite(files[i].chunks[k].last_count, files[i].chunks[k].last); 
                }                       }
            else {                                                          // if no changes and last chunk in the the last file                        
                Fwrite(files[i].chunks[k].last_count, files[i].chunks[k].last); 
            }
        }
            free(files[i].chunks[k].compressed);
            // printf("file %d chunk %d                free\n", i ,k);
            // printf("Allow one more chunk %d\n",makemorechunks);
            // pthread_mutex_lock(&mutex2);
            // makemorechunks++;
            // pthread_cond_signal(&cond2);
            // pthread_mutex_unlock(&mutex2);
        }
        // printf("file %d                free chunks\n", i);
        free(files[i].chunks);
        // printf("file %d                unmap\n", i );
        munmap(files[i].file, files[i].size);
        // printf("file %d                close\n", i );
        close(files[i].fd);
    }
}
