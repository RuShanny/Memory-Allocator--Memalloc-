#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

#include "typedefs.h"
#include "memalloc.h"

#define MEM_UNINITIALIZED 0x5a
#define MEM_AFTER_FREE    0x6b

struct uploaded_chunk
{
	void* addr;
	int get_counter;
	int release_counter;
	unsigned int id;
	unsigned long size;
	unsigned long size_aligned;
	uploaded_chunk* prev;
	uploaded_chunk* next;
};

struct uploaded_chunk_borders
{
	uploaded_chunk* prev;
	uploaded_chunk* next;
};


uploaded_chunk* search_chunk_with_given_id_in_memory(size_t id);
uploaded_chunk_borders* search_free_space_in_buffer(size_t size_aligned);
uploaded_chunk_borders* find_chunks_to_swap_out(size_t required_size);
void poison_memory_after_free(void* addr, size_t size);
void poison_uninitialized_memory(void* addr, size_t size);



#endif 
