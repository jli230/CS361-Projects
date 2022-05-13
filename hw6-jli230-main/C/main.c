#include<stdio.h>
#include<malloc.h>
#include<stdint.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

void print_chunk(FILE* out, void* ptr);
void print_heap(FILE* out, void* from);
void print_freelist(FILE* out, void* hdr);

// nasty trick to find the relevant free list header. this is highly unstable, 
// and only works when the freelist is empty.
//
// it creates two chunks (the second one to protect the first from being coalesced).
// frees the first one, and reads the address of the header from the freed chunk.
void* find_freelist(int size) {
    void* ptr = malloc(size);
    void* ptr2 = malloc(size);
    free(ptr);
    void *hdr = *((void**)ptr)+16;
    printf("%p\n", hdr);
    free(ptr2);
    return hdr;
}

int main() {
    // incidentally, this first call to fprintf also mallocs a buffer. We won't
    // see this one, since our bottom comes after that.
    fprintf(stdout,"Allocating a chunk to serve as the bottom of our heap.\n");
    void* bottom=malloc(1000);
    print_heap(stdout,bottom);
    fprintf(stdout,"\nCalling malloc_trim(0)\n");
    malloc_trim(0);
    print_heap(stdout,bottom);

    printf("\nMallocing some chunks\n");
    void* chunks[10];
    for(int i=0;i<5;i++) {
        chunks[i]=malloc(200);
        print_heap(stdout,bottom);
    }

    printf("\nNow freeing them in reverse order\n");
    for(int i=4;i>=0;i--) {
        free(chunks[i]);
        print_heap(stdout,bottom);
    }

    printf("\nMallocing some chunks again... \n");
    for(int i=0;i<5;i++) {
        chunks[i]=malloc(200);
    }

    printf("but now freeing them in the same order. Notice the difference.\n");
    for(int i=0;i<5;i++) {
        free(chunks[i]);
        print_heap(stdout,bottom);
    }

    printf("\nCalling malloc_trim(0)\n");
    malloc_trim(0);
    print_heap(stdout,bottom);

    for(int i=0;i<10;i++) {
        chunks[i]=malloc(200);
        memset(chunks[i],0xff,200); // to make debugging a little easier
    }

    void *hdr = find_freelist(200);

    printf("\nBuilding a freelist of 200 byte chunks\n");
    for(int i=0;i<10;i+=2) {
        free(chunks[i]);
        print_freelist(stdout,hdr);
    }

    printf("Freeing the rest\n");
    for(int i=1;i<10;i+=2) {
        free(chunks[i]);
        print_freelist(stdout,hdr);
    }


    printf("Building another freelist of 200 byte chunks\n");
    for(int i=0;i<10;i++) {
        chunks[i]=malloc(200);
        memset(chunks[i],0xff,200); // to make debugging a little easier
    }
    for(int i=0;i<10;i+=2) {
        free(chunks[i]);
        print_freelist(stdout,hdr);
    }

    printf("\nBut one single malloc, and the freelist looks like this:\n");
    malloc(1000);
    printf("\nIt's empty! Can you explain why? (hint: have a closer look at what happened when 'freeing the rest' above)");
    print_freelist(stdout,hdr);

}