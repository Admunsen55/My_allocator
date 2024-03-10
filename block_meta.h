#pragma once

#include <stdio.h>

/* Structura care sa retina datele unui bloc de memorie */
struct block_meta {
	size_t size;
	int status;
	struct block_meta *prev;
	struct block_meta *next;
};

/* Posibilile stari ale unui block */
#define STATUS_FREE   0
#define STATUS_ALLOC  1
#define STATUS_MAPPED 2
