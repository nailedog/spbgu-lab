#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t* data;
    size_t size;
    size_t capacity;
} ArrayList;

ArrayList* arraylist_create(void);
void arraylist_free(ArrayList* list);

void arraylist_push(ArrayList* list, uint32_t value);
uint32_t arraylist_pop(ArrayList* list);
uint32_t arraylist_get(const ArrayList* list, size_t index);
void arraylist_set(ArrayList* list, size_t index, uint32_t value);

size_t arraylist_size(const ArrayList* list);
void arraylist_clear(ArrayList* list);
void arraylist_resize(ArrayList* list, size_t new_size);
ArrayList* arraylist_clone(const ArrayList* list);
