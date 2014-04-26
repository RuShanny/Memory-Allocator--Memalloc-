#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "buffer_logic.h"

uploaded_chunk* search_chunk_with_given_id_in_memory(size_t id) {
    
    uploaded_chunk* chunk = mem_alloc->uploaded_chunks_list;

    while(chunk != NULL) 
    {
        if (chunk->id == id) {
            /* if the first chunk has this id, then return the pointer to the beginning of the list */
            if (chunk->prev == NULL) return chunk;
            else return chunk->prev->next;
        }

        chunk = chunk->next;
    }

    return NULL;
}


uploaded_chunk_borders* search_free_space_in_buffer(size_t size_aligned) {
    /* if swap_file is empty, create first chunk which starts from the beginning of the swap file */
    if (mem_alloc->uploaded_chunks_list == NULL) 
    {
    	//printf("\n\nuploaded_chunks_list is empty\n\n");
        uploaded_chunk_borders* upd_chunk_borders = (uploaded_chunk_borders*)malloc(sizeof(uploaded_chunk_borders));
        upd_chunk_borders->prev = NULL;
        upd_chunk_borders->next = NULL;
        return upd_chunk_borders;
    }

    uploaded_chunk* start = mem_alloc->uploaded_chunks_list;
    bool got_free_space = false;
    ptrdiff_t free_space = 0;


    /* if the chunks_list chunk starts not from the beginning of the buffer,
       there can be space available for a new chunk */
    free_space = start->addr - mem_alloc->buf_begin;
    if (free_space >= size_aligned) 
    {
    	//printf("\n\nchunk starts not from the beginning of the BUFFER\n\n");
    	uploaded_chunk_borders* upd_chunk_borders = (uploaded_chunk_borders*)malloc(sizeof(uploaded_chunk_borders));
    	upd_chunk_borders->prev = NULL;
        upd_chunk_borders->next = start;

        return upd_chunk_borders;
    } 

    /* search the available space between chunks in swap_file */
    while(start->next != NULL) 
    {
        free_space = start->next->addr - (start->addr + start->size_aligned);
        if (free_space >= size_aligned) 
        {
            got_free_space = true;
            break;
        }
        /* continue searching */
       start = start->next;
    }

    /* check if there is space available after the last chunk (till the end of the swap file) */
    if (got_free_space == false) 
    { 
        free_space = mem_alloc->buf_end - (start->addr + start->size_aligned);
        /* if there is necessairy free space, create chunk which will be the last in the swap file at this moment */
        if (free_space >= size_aligned) 
        {
        	uploaded_chunk_borders* upd_chunk_borders = (uploaded_chunk_borders*)malloc(sizeof(uploaded_chunk_borders));
    		upd_chunk_borders->prev = start;
        	upd_chunk_borders->next = NULL;
        	return upd_chunk_borders;
        }

        //printf("No free space awailable IN BUFFER at the moment. Try to alloc later\n");
        return NULL;
    }

    uploaded_chunk_borders* upd_chunk_borders = (uploaded_chunk_borders*)malloc(sizeof(uploaded_chunk_borders));
    upd_chunk_borders->prev = start;
    upd_chunk_borders->next = start->next;
 

    return upd_chunk_borders;
}


uploaded_chunk_borders* find_chunks_to_swap_out(size_t required_size) {
	bool is_continious_mem_block = false;
	size_t potential_free_space = 0;
	ptrdiff_t diff;

	//printf("find_chunks_to_swap_out begins\n");
	uploaded_chunk* chunk = mem_alloc->uploaded_chunks_list;
	uploaded_chunk_borders* upd_chunk_borders = (uploaded_chunk_borders*)malloc(sizeof(uploaded_chunk_borders));
	while (chunk != NULL) {
		if (chunk->get_counter == chunk->release_counter) {
			/* we should take into account that there can be a free place between two chunks 
			   that's why we do not consider only chunk->size_aligned */
			if (chunk->next != NULL) diff = chunk->next->addr - chunk->addr;
			else diff = mem_alloc->buf_end - chunk->addr;

			if (diff >= required_size) {
				/* if the size of this chunk is enough to fulfill the required size,
				   return this chunk */
				upd_chunk_borders->prev = chunk;
				upd_chunk_borders->next = upd_chunk_borders->prev;
				return upd_chunk_borders;
			}

			/* if there is still continious memory block of chunks, add size of this chunk 
			   and check if now there is enougth place to fulfill required size */
			if (is_continious_mem_block) {
				potential_free_space += diff;
				if (potential_free_space >= required_size) {
					upd_chunk_borders->next = chunk;
					return upd_chunk_borders;
				} 
			} else { /* if continious memory block of chunks has not started yet, 
					    let this chunk to be the first in this array */
				potential_free_space = diff;
				is_continious_mem_block = true;
				upd_chunk_borders->prev = chunk;
			}
		} else /* if this chunk interrupt a continious memory block of chunks, 
			      set boolean flag to false */
			is_continious_mem_block = false;
		chunk = chunk->next;
	}

	//printf("Cannot swap out any chunk\n");
	return NULL;
}


void poison_memory_after_free(void* addr, size_t size) {
    int i;
    char* addr_char = (char*)addr;
    for (i = 0; i < size; i++) {
        addr_char[i] = MEM_AFTER_FREE;
    }
}


void poison_uninitialized_memory(void* addr, size_t size) {
    int i;
    char* addr_char = (char*)addr;
    for (i = 0; i < size; i++) {
        addr_char[i] = MEM_UNINITIALIZED;
    }
}
