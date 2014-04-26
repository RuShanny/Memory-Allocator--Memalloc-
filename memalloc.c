#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "memalloc.h"


volatile int allocator_created = 0;
volatile int allocator_initialized = 0;

pthread_mutex_t mutex;


size_t ma_alloc(size_t size) {
	int size_aligned = align_to_pagesize(size);
	if (mem_alloc->mem_size < size_aligned) {
		perror("mem_alloc");
		errno = E2BIG;
		return 0;
	}

    pthread_mutex_lock(&mutex);
	swapped_chunk_borders* swp_chunk_borders = search_free_space_in_swap(align_to_pagesize(size));
	if (swp_chunk_borders == NULL) {
		perror("mem_alloc");
		errno = ENOMEM;
        pthread_mutex_unlock(&mutex);
		return 0;
	}

	if (mem_alloc->max_id >= UINT_MAX) {
		perror("mem_alloc");
		errno = EMLINK;
        pthread_mutex_unlock(&mutex);
		return 0;
	}

	size_t id = mem_alloc->max_id;

	create_and_register_chunk_in_swap(size, id, swp_chunk_borders);
	mem_alloc->max_id++;

    pthread_mutex_unlock(&mutex);

	return id;
}


void* ma_get(size_t id) {
	/* check if id is a valid number */
	if ((id <= 0) || (id >= UINT_MAX)) {
		perror("ma_get");
		errno = EINVAL;
		return NULL;
	}

    pthread_mutex_lock(&mutex);
	uploaded_chunk* upd_chunk = search_chunk_with_given_id_in_memory(id);

	if (upd_chunk != NULL) {
		upd_chunk->get_counter++;
        pthread_mutex_unlock(&mutex);
		return upd_chunk->addr;
	}

	swapped_chunk* swp_chunk = search_chunk_with_given_id_in_swap(id);
	if (swp_chunk == NULL) {
		perror("ma_get");
		errno = EFAULT;
        pthread_mutex_unlock(&mutex);
		return NULL;
	}

	uploaded_chunk_borders* upd_chunk_borders = search_free_space_in_buffer(swp_chunk->size_aligned);
	if (upd_chunk_borders == NULL) {
		upd_chunk_borders = find_chunks_to_swap_out(swp_chunk->size_aligned);
		if (upd_chunk_borders == NULL) {
			errno = ENOMEM;
            pthread_mutex_unlock(&mutex);
			return NULL;
		}

		uploaded_chunk* chunk = upd_chunk_borders->prev;

		/* in order to use this borders to map our chunk, we should redefine it */
		upd_chunk_borders->prev = upd_chunk_borders->prev->prev;

		uploaded_chunk* next_chunk;
		do {
			next_chunk = chunk->next;
			if (swap_out_chunk(chunk) == 0)
				printf("%s\n", strerror(errno));
			chunk = next_chunk;
		} while (chunk != upd_chunk_borders->next->next);

		/* in order to use this borders to map our chunk, we should redefine it */
		upd_chunk_borders->next = chunk;
        pthread_mutex_unlock(&mutex);
	}



	void* addr = swap_in_chunk(swp_chunk, upd_chunk_borders);
 	if (addr == NULL) {
 		printf("%s\n", strerror(errno));
        pthread_mutex_unlock(&mutex);
 		return NULL;
 	} 

    pthread_mutex_unlock(&mutex);
	return addr;
}


int ma_release(size_t id) {
    pthread_mutex_lock(&mutex);

	uploaded_chunk* upd_chunk = search_chunk_with_given_id_in_memory(id);
	if ((upd_chunk == NULL) || (upd_chunk->get_counter == upd_chunk->release_counter)) {
		perror("ma_release");
		errno = EFAULT;
        pthread_mutex_unlock(&mutex);
		return 0;
	}

	upd_chunk->release_counter++;
    pthread_mutex_unlock(&mutex);
	return 1;
}

int ma_free(size_t id) {

    pthread_mutex_lock(&mutex);

	uploaded_chunk* upd_chunk = search_chunk_with_given_id_in_memory(id);
    
    //printf("search_chunk_with_given_id_in_memory after\n");
	
	if ((upd_chunk != NULL) && (upd_chunk->get_counter != upd_chunk->release_counter)) {
		perror("ma_free");
		errno = EFAULT;
        pthread_mutex_unlock(&mutex);
		return 0;
	} 


	if (upd_chunk != NULL) {
		if (msync(upd_chunk->addr, upd_chunk->size_aligned, MS_SYNC) == -1) {
			perror("msync");
			pthread_mutex_unlock(&mutex);
			return 0;
		}
        if (upd_chunk->addr == NULL) printf("addr IS NULL\n");
        poison_memory_after_free(upd_chunk->addr, upd_chunk->size_aligned);
    }
    //printf("poison_memory_after_free after\n");
	
	if (swap_out_chunk(upd_chunk) == 0) {
		printf("%s\n", strerror(errno));
		perror("ma_free");
		errno = EFAULT;
        pthread_mutex_unlock(&mutex);
		return 0;
	}

    //printf("swap_out_chunk after\n");

	swapped_chunk* swp_chunk = search_chunk_with_given_id_in_swap(id);
	if (swp_chunk == NULL) {
		perror("ma_free");
		errno = EFAULT;
        pthread_mutex_unlock(&mutex);
		return 0;
	}

    //printf("search_chunk_with_given_id_in_swap after\n");

	delete_from_swap_file(swp_chunk);

    //printf("delete_from_swap_file after\n");
    
    pthread_mutex_unlock(&mutex);
	return 1;
}




int ma_init(size_t mem, size_t swap, const char* swap_path) {
    if (__sync_bool_compare_and_swap(&(allocator_created), 0, 1)) {
        /* if allocator is not created yet, do it */
    	size_t aligned_size = align_to_pagesize(mem);
    	int fd;
    	void* buf;

    	int returned_value = posix_memalign(&buf, sysconf(_SC_PAGESIZE), aligned_size);
    	/* in case of an error posix_memalign() does not set errno, so we should do it ourself */
        if (returned_value != 0) {
        	errno = ENOMEM;
        	return 0;
        }

        poison_uninitialized_memory(buf, aligned_size);

        /* we give access rights only to user, not for group or others */
        if ((fd = open(swap_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
        	perror("open");
            return 0;
        }

        returned_value = posix_fallocate(fd, 0, (off_t)swap);
        /* in case of an error posix_fallocate() does not set errno, so we should do it ourself */
        if (returned_value != 0) {
        	perror("posix_fallocate");
        	switch(returned_value) {
        		case ENOSPC: errno = ENOSPC; break;
        		case EFBIG: errno = EFBIG; break;
        		default: errno = EFAULT; break;
        	}
        	/* return 0 in case of an error */
        	return 0;
        }

        mem_alloc = (allocator*)malloc(sizeof(allocator));
        mem_alloc->mem_size = aligned_size;
        mem_alloc->swap_size = swap;
        mem_alloc->buf_begin = buf;
        mem_alloc->buf_end = buf + aligned_size - 1;
        mem_alloc->max_id = 1;
        mem_alloc->swap_fd = fd;
        mem_alloc->swapped_chunks_list = NULL;
        mem_alloc->uploaded_chunks_list = NULL;

        pthread_mutex_init(&mutex, NULL);
        allocator_initialized = 1;

    } else { 
        /* allocator is already created, but we should check if it is initialized. If not, we should wait its initialization */
        while (!allocator_initialized) {}
        printf("Waitinig for allocator to be initialized\n");
    }
    /* return 1 on SUCCESS */
    return 1;

}


void ma_deinit() {
    /* if allocator has been already deinitialized, there is nothing to do */
    if (mem_alloc == NULL) return;


	/* close swap file */ 
	if (close(mem_alloc->swap_fd) != 0) {
		//printf("close: %s\n", strerror(errno));
	    return;
	}

	/* free buffer */
 	if (mem_alloc->buf_begin != NULL) free(mem_alloc->buf_begin);

	/* free lists */
	uploaded_chunk *upd_chunk_next;
	while (mem_alloc->uploaded_chunks_list != NULL) {
		upd_chunk_next = mem_alloc->uploaded_chunks_list->next;
		free(mem_alloc->uploaded_chunks_list);
		mem_alloc->uploaded_chunks_list = upd_chunk_next;
	}

	swapped_chunk* swp_chunk_next;
	while (mem_alloc->swapped_chunks_list != NULL) {
		swp_chunk_next = mem_alloc->swapped_chunks_list->next;
		free(mem_alloc->swapped_chunks_list);
		mem_alloc->swapped_chunks_list = swp_chunk_next;
	}

	pthread_mutex_destroy(&mutex);
	free(mem_alloc);
}
