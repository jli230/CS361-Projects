#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<stdint.h>
#include<unistd.h>
#define chunk_size(chunk) (*(((unsigned long*)chunk)- 1)& (~ (unsigned long)0x7))
#define is_allocated(chunk) (*((unsigned long*)((void*)chunk+chunk_size(chunk)-8))&1)


void print_chunk(FILE* out, void* ptr) {
    
    //unsigned long * newptr = (unsigned long *) ptr;
    void * chunk = ptr-8;
    //void* chunkptr = (uint64_t *) chunk;
    //typedef struct malloc_chunk* mchunkptr = ptr;
    //metadata_t *chunkData = ptr;
    int ptr_size;
    void* top = sbrk(0);
    //ptr_size = malloc_usable_size(ptr);
    //ptr_size = mchunkptr -> mchunk_size;
    //unsigned long * test = &newptr-24;
    //char fstring[100];
    //printf("%p\n", newptr);
    
    
    void* next = chunk+(*((uint64_t*)chunk) & ~3l);
    
    if (next < top) {
        int allocated = is_allocated(ptr);
        //printf("%i", allocated);
        if (allocated == 1) {
            fprintf(out, "[A%li: %p]", *((uint64_t*)chunk) & ~3l, chunk); 
        } else if (allocated == 0 ) {
            fprintf(out, "[F%li: %p]", *((uint64_t*)chunk) & ~3l, chunk); 
        }
        //fputs(fstring, out);
    } else {
        fprintf(out, "[F%li: %p]", (*((uint64_t *)chunk) & ~3l), chunk); 
            // prinft "F"
    }
    // if (next < top) { 
    // printf("%li\n", (*((uint64_t *)chunk) & ~3l));
    // printf("%08lx\n", *((uint64_t *)next));
    // printf("%ld\n", (*((uint64_t *)next) & 1l) );
    // }
    //printf("%i\n", *((unsigned long long*)ptr - 1) & ~0xFFF1);
    //printf("Test Pointer: %i\n", *test);
    //sprintf(fstring, "<%i>: %p\n", ptr_size, ptr); 
    //fputs(fstring, out);
}
void print_heap(FILE* out, void* from) {
    void * top = sbrk(0);
    void * chunk = from - 8;
    while (chunk < top) {
        void* next = chunk + (*((uint64_t*)chunk) & ~3l); 
        print_chunk(out, chunk+8);
        chunk = next;
        // printf("%p\n", chunk);
    }
    fprintf(out, "\n");
}
void print_freelist(FILE* out, void* hdr) {
    void *temp = *((void **)hdr)+16;
    while (temp != hdr) {
       print_chunk(out, temp);
       temp = *((void **)temp)+16;
    }
    fprintf(out,"\n");
}
