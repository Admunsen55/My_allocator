#include <unistd.h>
#include <sys/mman.h>
#include "define.h"
#include "block_meta.h"

block_d *front;
block_d *rear;

size_t minimun(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

// functie care determina dimensiunea aliniata (cu tot cu padding)
size_t get_appropiate_size(size_t requested_size)
{
    size_t remainder = requested_size % ALIGN_VAL;

    if (remainder != 0)
        return requested_size + (ALIGN_VAL - remainder);

    return requested_size;
}

// functie care insereaza un block metadata in lista imediat dupa "block_before"
void insert_block_in_list(block_d *block_before, block_d *new_block, size_t block_size, int status)
{
    // verificam daca lista e nula
    if (block_before == NULL) {
        // initializam lista cu primul element in caz afirmativ
        front = new_block;
        rear = new_block;
        new_block->prev = NULL;
        new_block->next = NULL;
    } else {
        // adaugam noul element in lista
        if (block_before->next != NULL)
            block_before->next->prev = new_block;
        new_block->next = block_before->next;
        block_before->next = new_block;
        new_block->prev = block_before;
        if (block_before == front)
            front = new_block;
    }
    // setam dimensiunea si statusul noului bloc
    new_block->size = block_size;
    new_block->status = status;
}

/* functie care adauga (insereaza) un bloc la inceputul sau finalul listei (in functie
de parametrul position care poate fi LEFT sau RIGHT) */
void add_block_to_list(block_d* new_block, size_t block_size, int status, int position)
{
    if (position == RIGHT)
        insert_block_in_list(front, new_block, block_size, status);
    else if (position == LEFT)
        insert_block_in_list(rear, new_block, block_size, status);
    else
        printf_("Incorrect positioning !!!\n");
}

// functie care scoate blocul metadata din lista si reface "legaturile"
void remove_block(block_d* block)
{
    if (front == rear) {
        front = NULL;
        rear = NULL;
        return;
    }
    // refacem legaturile in functie de caz
    if (block == front) {
        front = block->prev;
        block->prev->next = block->next;
    } else if (block == rear) {
        rear = block->next;
        block->next->prev = block->prev;
    } else {
        block->prev->next = block->next;
        block->next->prev = block->prev;
    }
}

/* functie care aloca memorie pe heap (metadata + data) si returneaza un
pointer catre zona datelor */
void *alloc_brk(size_t requested_size)
{
    block_d *new_block;
    size_t real_size = get_appropiate_size(requested_size);
    size_t struc_size = sizeof(block_d);

    new_block = sbrk(real_size + struc_size);
    // daca alocarea a avut succes adaugam in lista blocul metadata
    if (new_block != NULL) {
        add_block_to_list((block_d*)new_block, real_size, STATUS_FREE, RIGHT);
        return (void*)new_block + struc_size;
    }

    return NULL;
}

// analog cu functia anterioara, dar pentru allocarile cu mmap
void *alloc_mmap(size_t requested_size)
{
    block_d *new_block;
    size_t real_size, struc_size = sizeof(block_d);
    real_size = get_appropiate_size(requested_size + struc_size);

    new_block = (block_d*)mmap(NULL, real_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_block != NULL) {
        add_block_to_list(new_block, real_size - struc_size, STATUS_MAPPED, LEFT);
        return (void*)new_block + struc_size;
    }

    return NULL;
}

// functie care prealoca pe heap 128KB
void *heap_preallocation()
{
    size_t prealloc_size_without_metadata = PREALLOCATION_SIZE - sizeof(block_d);
    return alloc_brk(prealloc_size_without_metadata);
}

// functie care face "merge" la doua blocuri adiacente 
void merge_adj_heap_blocks(block_d *first_block)
{
    block_d *second_block;
    size_t struc_size = sizeof(block_d);
    second_block = (block_d*)((void*)first_block + struc_size + first_block->size);
    first_block->size += struc_size + second_block->size;
    remove_block(second_block);
}

/* functia de "coalesce" (unire blocuri) (daca primeste parametrul DONT_STOP, atunci functia
face "coalesce" pentru toate blocurile din lista pentru care se poate face, daca
primeste parametrul STOP, atunci functia face coalesce pana la primul bloc cu
status alloc) */
void coalesce_blocks(block_d *start, int stop_at_status_alloc)
{
    block_d *cur = start, *prev = NULL;

    while (cur != NULL) {
        if (prev != NULL) {
            if (cur->status == STATUS_FREE && prev->status == STATUS_FREE) {
                merge_adj_heap_blocks(prev);
                cur = prev->next;
                continue;
            }
        }
        prev = cur;
        cur = cur->next;
        if (stop_at_status_alloc) {
            if (cur != NULL) {
                if (cur->status == STATUS_ALLOC) {
                    return;
                }
            }
        }
    }
}

/* functie care cauta cel mai bun bloc liber din lista si returneaza
un pointer catre acesta */
block_d *find_best(size_t requested_size)
{
    size_t best_differnece = PREALLOCATION_SIZE + 1, difference, real_size;
    block_d *best_block = NULL, *cur = rear;

    real_size = get_appropiate_size(requested_size);
    while (cur != NULL) {
        if (cur->status == STATUS_FREE && cur->size >= real_size) {
            difference = cur->size - real_size;
            if (difference < best_differnece) {
                best_differnece = difference;
                best_block = cur;
            }
        }
        cur = cur->next;
    }

    return best_block;
}

/* functie de split care reduce blocul curent la dimensiunea necesara
si creeaza un nou bloc liber (daca este cazul) */
void split_block(block_d *block_to_split, size_t requested_size)
{
    size_t difference, struc_size = sizeof(block_d), real_size;
    block_d *new_block;

    real_size = get_appropiate_size(requested_size);
    if (block_to_split->size < real_size || block_to_split->status == STATUS_MAPPED) {
        printf_("Incorectely chosen block\n");
        return;
    }

    if (block_to_split->size >= real_size + struc_size + ALIGN_VAL) {
        difference = block_to_split->size - real_size - struc_size;
        block_to_split->size = real_size;

        new_block = (block_d*)((void*)block_to_split + struc_size + real_size);
        insert_block_in_list(block_to_split, new_block, difference, STATUS_FREE);
    }
}

// functie care expandeaza ultimul bloc
void expand_last_block(size_t size)
{
    size_t struc_size = sizeof(block_d);
    alloc_brk(size - front->size - struc_size);
    merge_adj_heap_blocks(front->prev);
}

void free_block(block_d *block_to_free)
{
    if (block_to_free->status == STATUS_ALLOC) {
        block_to_free->status = STATUS_FREE;
    } else if (block_to_free->status == STATUS_MAPPED) {
        remove_block(block_to_free);
        munmap((void*)block_to_free, block_to_free->size + sizeof(block_d));
    } else {
        printf_("Can't be freed\n");
    }
}
