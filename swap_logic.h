#ifndef SWAP_LOGIC_H
#define SWAP_LOGIC_H

#include <stdlib.h>

#include "typedefs.h"
#include "memalloc.h"

struct swapped_chunk {
	unsigned int id;
	size_t offset;
	unsigned long size;
	unsigned long size_aligned;
	swapped_chunk* prev;
	swapped_chunk* next;
};

struct swapped_chunk_borders {
	swapped_chunk* prev;
	swapped_chunk* next;
};


swapped_chunk* search_chunk_with_given_id_in_swap(size_t id);
void create_and_register_chunk_in_swap (size_t size, unsigned int id, swapped_chunk_borders* swp_chunk_borders);
swapped_chunk_borders* search_free_space_in_swap(size_t size_aligned);

#endif