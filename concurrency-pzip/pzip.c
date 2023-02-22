#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <assert.h>
#include <string.h>
#define INT_SIZE 4
#define CHAR_SIZE 1

typedef struct chunk {
    char *start;         // start of the chunk uncompressed
    char *compressed;    // compressed chunk
    int compressed_size; // size of the compressed chunk
    int size;            // number of bytes of the chunk
    int first_count;     // number of times the first character appears
    int last_count;      // number of times the last character appears
    int changes;         // number of different characters in the chunk - 1
    char first;          // first character in the chunk
    char last;           // last character in the chunk
}chunk;

void compress_chunk(chunk *c)
{
    int count = 1;
    for (int i = 0; i < c->size-1; i++) {
        if (c->start[i] == c->start[i+1]) {
            count++;
        }
        else {
            c->changes++;
            count = 1;
            // write to the compressed chunk
            memcpy(c->compressed+c->compressed_size, &count, INT_SIZE);
            c->compressed_size += INT_SIZE;
            memcpy(c->compressed+c->compressed_size, &c->start[i], CHAR_SIZE);
            c->compressed_size += CHAR_SIZE;
        }
        if (c->changes == 0){
            c->first_count = count;
        }
    }
    c->last_count = count;
}
int main(int argc, char *argv[])
{
    // NO. of cores
    int cores = get_nprocs();
    // No. of files
    int files = argc - 1;

    // Calculating the total size of files
    size_t total_size = 0;
    for (int i=1; i<argc; i++) {
        struct stat st;
        stat(argv[i], &st);
        total_size += st.st_size;
    }

    // Allocating memory for the files to be contiguous
    char *uncompressedFiles = (char *)malloc(total_size);

    // Reading the files and copying them to the contiguous memory
    size_t offset = 0;

    for (int i=1; i<argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        struct stat st;
        fstat(fd, &st);

        // Place the file in order in the contiguous memory block.
        char *file = (char *)
            mmap(uncompressedFiles+offset, // the location of the file in the contiguous memory block
            st.st_size,                    // the size of the file
            PROT_READ,                     // read only
            MAP_PRIVATE,                   // private
            fd,                            // the file descriptor
            0);                            // the offset of the file.
        assert(file != MAP_FAILED);
        offset += st.st_size; // increment the offset by the size of the file
    }

    // TODO: deduce chunk size from the total size of the files depending on the number of cores
    int chunksize;
    int chunkcount;

    // Array of chunks
    chunk chunks[chunkcount];

    for (int i=0; i<chunkcount; i++) {
        chunks[i].compressed = (char *)malloc((INT_SIZE+CHAR_SIZE)*chunksize); // the maximum size of the compressed chunk
        assert(chunks[i].compressed != NULL);
        chunks[i].compressed_size = 0;
        chunks[i].first = uncompressedFiles[i*chunksize];
        chunks[i].last = uncompressedFiles[(i+1)*chunksize-1];
        chunks[i].first_count = 0;
        chunks[i].last_count = 0;
        chunks[i].changes = 0;
        chunks[i].start = uncompressedFiles + (i * chunksize);
        if (i == chunkcount-1) {
            chunksize = total_size - (i * chunksize);
            break;
        }
        chunks[i].size = chunksize;
        // Add the chunks to the queue
    }
}
