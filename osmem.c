#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "define.h"
#include "block_meta.h"
#include "osmem.h"

// preallocation = 0;
int preallocation;

void *os_malloc(size_t size)
{
	size_t struc_size = sizeof(block_d);
	block_d *metadata;
	void *ptr = NULL;

	if (size == 0) {
		return NULL;
	}
	if (size + struc_size >= MMAP_TRESHOLD) {
		ptr = alloc_mmap(size);
	} else {
		if (preallocation == 0) {
			ptr = heap_preallocation();
			preallocation = 1;
		}
		coalesce_blocks(rear, DONT_STOP);
		metadata = find_best(size);
		if (metadata != NULL) {
			split_block(metadata, size);
			metadata->status = STATUS_ALLOC;
			ptr = (void*)metadata + struc_size;
		} else {
			if (front->status == STATUS_FREE) {
				expand_last_block(size);
				ptr = (void*)front + struc_size;	
			} else {
				ptr = alloc_brk(size);
			}
			front->status = STATUS_ALLOC;
		}
	}

	return ptr;
}

void os_free(void *ptr)
{
	if (ptr == NULL) {
		return;
	}
	size_t struc_size = sizeof(block_d);
	block_d *metadata = (block_d*)(ptr - struc_size);
	free_block(metadata);
}

void *os_calloc(size_t nmemb, size_t size)
{
	size_t struc_size = sizeof(block_d), real_size = nmemb * size;
	block_d *metadata;
	void *ptr = NULL;

	if (nmemb == 0 || size == 0) {
		return NULL;
	}
	if (real_size + struc_size >= PAGE_SIZE) {
		ptr = alloc_mmap(real_size);
	} else {
		if (preallocation == 0) {
			ptr = heap_preallocation();
			preallocation = 1;
		}
		coalesce_blocks(rear, DONT_STOP);
		metadata = find_best(real_size);
		if (metadata != NULL) {
			split_block(metadata, real_size);
			metadata->status = STATUS_ALLOC;
			ptr = (void*)metadata + struc_size;
		} else {
			if (front->status == STATUS_FREE) {
				expand_last_block(real_size);
				ptr = (void*)front + struc_size;	
			} else {
				ptr = alloc_brk(real_size);
			}
			front->status = STATUS_ALLOC;
		}
	}
	memset(ptr, 0, real_size);

	return ptr;
}

void *os_realloc(void *ptr, size_t size)
{
	size_t struc_size = sizeof(block_d);
	block_d *metadata;
	void *new_ptr = NULL;

	if (size == 0) {
		os_free(ptr);
		return NULL;
	}
	if (ptr == NULL) {
		return os_malloc(size);
	}
	metadata = (block_d*)(ptr - struc_size);
	if (metadata->status == STATUS_FREE) {
		return NULL;
	} else if (metadata->status == STATUS_MAPPED) {
		new_ptr = os_malloc(size);
		memcpy(new_ptr, ptr, minimun(metadata->size, size));
		free_block(metadata);
	} else if (metadata->status == STATUS_ALLOC) {
		if (metadata->size >= size) {
			split_block(metadata, size);
			return ptr;
		}
		if (metadata == front) {
			expand_last_block(size);
			return ptr;
		} else {
			while (metadata->next->status == STATUS_FREE) {
				merge_adj_heap_blocks(metadata);
				if (metadata->size >= size) {
					split_block(metadata, size);
					return ptr;
				}
				if (metadata->next == NULL) {
					break;
				}
			}
			new_ptr = os_malloc(size);
			memcpy(new_ptr, ptr, minimun(metadata->size, size));
			free_block(metadata);
		}
	} else {
		// caz prost (de adaugat ceva flag)
	}

	return new_ptr;
}
