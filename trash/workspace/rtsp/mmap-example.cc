#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct Mmap 
{
    int fd;
    void* mptr;

    Mmap(const char* fn) {
        struct stat sb;
        fd = open(fn, O_RDONLY);
        fstat(fd, &sb); // printf("Size: %d\n", (int)sb.st_size);
        mptr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    }
};

int main(int argc, char *argv[])
{
    //const char *memblock;
    //int i, fd;
    //struct stat sb;

    //fd = open(argv[1], O_RDONLY);
    //fstat(fd, &sb);
    //printf("Size: %d\n", (int)sb.st_size);

    //memblock = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    //if (memblock == MAP_FAILED)
    //    handle_error("mmap");

    Mmap m(argv[1]);
    for (int i = 0; i < 10; i++) {
        printf("[%d]=%X ", i, ((char*)m.mptr)[i]);
    }
    printf("\n");
    return 0;
}

