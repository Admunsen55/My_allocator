#pragma once

#include <stdio.h>

/* Structure to hold memory block metadata */
struct block_meta {
	size_t size;
	int status;
	struct block_meta *prev;
	struct block_meta *next;
};

/* Block metadata current status values */
#define STATUS_FREE   0
#define STATUS_ALLOC  1
#define STATUS_MAPPED 2
