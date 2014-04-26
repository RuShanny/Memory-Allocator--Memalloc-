#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


#include "typedefs.h"
#include "swap_logic.h"

void delete_from_swap_file(swapped_chunk* swp_chunk) {
    if (swp_chunk->prev != NULL)
        swp_chunk->prev->next = swp_chunk->next;
    if (swp_chunk->next != NULL)
        swp_chunk->next->prev = swp_chunk->prev;
    if ((swp_chunk->prev == NULL)&&(swp_chunk->next == NULL))
        mem_alloc->swapped_chunks_list = NULL;
}

swapped_chunk* search_chunk_with_given_id_in_swap(size_t id) {
	swapped_chunk* chunk = mem_alloc->swapped_chunks_list;

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


void create_and_register_chunk_in_swap (size_t size, unsigned int id, swapped_chunk_borders* swp_chunk_borders) {
    //printf("create_and_register_chunk_in_swap begins\n");

    swapped_chunk* swp_chunk = (swapped_chunk*)malloc(sizeof(swapped_chunk));
    swp_chunk->id = id;
    swp_chunk->size = size;
    swp_chunk->size_aligned = align_to_pagesize(size);
    swp_chunk->prev = swp_chunk_borders->prev;
    swp_chunk->next = swp_chunk_borders->next;

    if (swp_chunk_borders->prev == NULL) 
    {
    	/* if this chunk is supposed to be the first in swap file, calculate offset respectively */
    	//printf("FIRST ELEMENT CREATED with id = %u\n", (unsigned int)id);
        mem_alloc->swapped_chunks_list = swp_chunk;
        swp_chunk->offset = 0;
    } else {
    	swp_chunk->offset = swp_chunk_borders->prev->offset + swp_chunk_borders->prev->size_aligned;
    	swp_chunk_borders->prev->next = swp_chunk;
    }


	if (swp_chunk_borders->next != NULL) 
		swp_chunk_borders->next->prev = swp_chunk;


    //printf("create_and_register_chunk_in_swap ends\n");
}


/* 
   searches free space in SWAP_FILE for a new chunk
   if it is found, return struct chunk_borders which contains pointers to chunk-neighbours
   if it is not fount, return NULL 
*/
swapped_chunk_borders* search_free_space_in_swap(size_t size_aligned) {
    
    //printf("search_free_space_in_swap begins\n");
    
    /* if swap_file is empty, create first chunk which starts from the beginning of the swap file */
    if (mem_alloc->swapped_chunks_list == NULL) 
    {
    	//printf("\n\nswapped_chunks_list is empty\n\n");
        swapped_chunk_borders* swp_chunk_borders = (swapped_chunk_borders*)malloc(sizeof(swapped_chunk_borders));
        swp_chunk_borders->prev = NULL;
        swp_chunk_borders->next = NULL;
        //printf("search_free_space_in_swap ends\n");
        return swp_chunk_borders;
    }

    swapped_chunk* start = mem_alloc->swapped_chunks_list;
    bool got_free_space = false;
    size_t free_space = 0;


    /* if the chunks_list chunk starts not from the beginning of the swap_file,
       there can be space available for a new chunk */
    if (mem_alloc->swapped_chunks_list->offset >= size_aligned) 
    {
    	//printf("mem_alloc->swapped_chunks_list->offset = %u\n", (unsigned int)mem_alloc->swapped_chunks_list->offset);
    	//printf("\n\nchunk starts not from the beginning of the swap_file\n\n");
    	swapped_chunk_borders* swp_chunk_borders = (swapped_chunk_borders*)malloc(sizeof(swapped_chunk_borders));
        swp_chunk_borders->prev = NULL;
        swp_chunk_borders->next = mem_alloc->swapped_chunks_list;


        return swp_chunk_borders;
    } 

    /* search the available space between chunks in swap_file */
    while(mem_alloc->swapped_chunks_list->next != NULL) 
    {
        free_space = mem_alloc->swapped_chunks_list->next->offset - mem_alloc->swapped_chunks_list->offset - mem_alloc->swapped_chunks_list->size_aligned;
        if (free_space >= size_aligned) 
        {
            got_free_space = true;
            break;
        }
        /* continue searching */
        mem_alloc->swapped_chunks_list = mem_alloc->swapped_chunks_list->next;
    }

    /* check if there is space available after the last chunk (till the end of the swap file) */
    if (got_free_space == false) 
    { 
        free_space = mem_alloc->swap_size - (mem_alloc->swapped_chunks_list->offset + mem_alloc->swapped_chunks_list->size_aligned);
        /* if there is necessairy free space, create chunk which will be the last in the swap file at this moment */
        if (free_space >= size_aligned) 
        {
        	swapped_chunk_borders* swp_chunk_borders = (swapped_chunk_borders*)malloc(sizeof(swapped_chunk_borders));
        	swp_chunk_borders->prev = mem_alloc->swapped_chunks_list;
        	swp_chunk_borders->next = NULL;

        	mem_alloc->swapped_chunks_list = start; 
        	return swp_chunk_borders;
        }

        //printf("No free space awailable at the moment. Try to alloc later\n");
        mem_alloc->swapped_chunks_list = start;
        return NULL;
    }

    swapped_chunk_borders* swp_chunk_borders = (swapped_chunk_borders*)malloc(sizeof(swapped_chunk_borders));
    swp_chunk_borders->prev = mem_alloc->swapped_chunks_list;
    swp_chunk_borders->next = mem_alloc->swapped_chunks_list->next;
   
    mem_alloc->swapped_chunks_list = start; 
    //printf("search_free_space_in_swap ends\n");

    return swp_chunk_borders;
}
