#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "swap_operations.h"

size_t align_to_pagesize(size_t required_size) {
    long int pagesize = sysconf(_SC_PAGESIZE);
    return (((~(pagesize - 1))&required_size) + pagesize);
}


void* swap_in_chunk (swapped_chunk* swp_chunk, uploaded_chunk_borders* upd_chunk_borders) {
    //printf("create_and_register_chunk_in_BUFFER begins\n");

    uploaded_chunk* upd_chunk = (uploaded_chunk*)malloc(sizeof(uploaded_chunk));
    upd_chunk->id = swp_chunk->id;
    upd_chunk->size = swp_chunk->size;
    upd_chunk->size_aligned = swp_chunk->size_aligned;
    upd_chunk->prev = upd_chunk_borders->prev;
    upd_chunk->next = upd_chunk_borders->next;

    if (upd_chunk_borders->prev == NULL) 
    {
         // if this chunk is supposed to be the first in swap file, calculate offset respectively
        printf("FIRST ELEMENT CREATED with id IN BUFFER = %u\n", (unsigned int)upd_chunk->id);
        mem_alloc->uploaded_chunks_list = upd_chunk;
        upd_chunk->addr = mem_alloc->buf_begin;
    } else {
        upd_chunk->addr = upd_chunk_borders->prev->addr + upd_chunk_borders->prev->size_aligned;
        upd_chunk_borders->prev->next = upd_chunk;
    }


    if (upd_chunk_borders->next != NULL) 
        upd_chunk_borders->next->prev = upd_chunk;

    upd_chunk->addr = mmap((caddr_t)upd_chunk->addr, upd_chunk->size_aligned, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, mem_alloc->swap_fd, (off_t)swp_chunk->offset);
    if (upd_chunk->addr == (caddr_t)(-1)) {
        perror("mmap");
        return NULL;
    }

    upd_chunk->get_counter = 1;
    upd_chunk->release_counter = 0;
    //printf("create_and_register_chunk_in_BUFFER ends\n");
}


int swap_out_chunk(uploaded_chunk* upd_chunk) {
    if (upd_chunk == NULL)
        // nothing to unmap 
        return 1;

    // may be it should be handled in another way
    if (munmap((caddr_t)upd_chunk->addr, upd_chunk->size_aligned) == -1) { 
        return 0;
    }

    if (upd_chunk->prev != NULL) upd_chunk->prev->next = upd_chunk->next;
    else mem_alloc->uploaded_chunks_list = upd_chunk->next;

    if (upd_chunk->next != NULL)
        upd_chunk->next->prev = upd_chunk->prev;

    if ((upd_chunk->prev == NULL)&&(upd_chunk->next == NULL))
        mem_alloc->uploaded_chunks_list = NULL;
    return 1;
}
