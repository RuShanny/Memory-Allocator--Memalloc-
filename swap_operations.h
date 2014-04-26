#ifndef SWAP_OPERATIONS_H
#define SWAP_OPERATIONS_H

#include <stdio.h>

#include "typedefs.h"
#include "swap_logic.h"
#include "buffer_logic.h"

size_t align_to_pagesize(size_t required_size);
void* swap_in_chunk (swapped_chunk* swp_chunk, uploaded_chunk_borders* upd_chunk_borders);
int swap_out_chunk(uploaded_chunk* upd_chunk);


#endif