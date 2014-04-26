#ifndef MEMALLOC_H
#define MEMALLOC_H


#include <stdio.h>

#include "typedefs.h"
#include "buffer_logic.h"
#include "swap_logic.h"
#include "swap_operations.h"

#define _FILE_OFFSET_BITS 64
extern allocator_created;
extern allocator_initialized;

struct allocator {
    size_t mem_size;
    size_t swap_size;
    void* buf_begin;
    void* buf_end;
    unsigned int max_id;
    int swap_fd;
    swapped_chunk* swapped_chunks_list;
    uploaded_chunk* uploaded_chunks_list;
};

allocator* mem_alloc;

size_t ma_alloc(size_t size);
void* ma_get(size_t id);
int ma_release(size_t id);
int ma_free(size_t id);
int ma_init(size_t mem, size_t swap, const char* swap_path);
void ma_deinit() ;

#endif
