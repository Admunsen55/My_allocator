#ifndef DEFINE_H
#define DEFINE_H

typedef struct block_meta block_d;

extern block_d *front;
extern block_d *rear; 

size_t minimun (size_t a, size_t b);
void *alloc_brk(size_t requested_size);
void *alloc_mmap(size_t requested_size);
void *heap_preallocation();
void coalesce_blocks(block_d *start, int stop_at_status_alloc);
block_d *find_best (size_t requested_size);
void merge_adj_heap_blocks(block_d *first_block);
void split_block(block_d *block, size_t requested_size);
void expand_last_block (size_t size);
void free_block(block_d *block_to_free);

#define LEFT 0
#define RIGHT 1
#define DONT_STOP 0
#define STOP 1
#define ALIGN_VAL 8
#define PAGE_SIZE 4096
#define MMAP_TRESHOLD 131072
#define PREALLOCATION_SIZE 131072

#endif
