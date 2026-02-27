#include "arraylist.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8

ArrayList* arraylist_create(void) {
    ArrayList* list = (ArrayList*)malloc(sizeof(ArrayList));
    if (!list) return NULL;

    list->data = (uint32_t*)malloc(INITIAL_CAPACITY * sizeof(uint32_t));
    if (!list->data) {
        free(list);
        return NULL;
    }

    list->size = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

void arraylist_free(ArrayList* list) {
    if (list) {
        if (list->data) {
            free(list->data);
        }
        free(list);
    }
}

static void arraylist_grow(ArrayList* list) {
    size_t new_capacity = list->capacity * 2;
    uint32_t* new_data = (uint32_t*)realloc(list->data, new_capacity * sizeof(uint32_t));
    if (!new_data) return;

    list->data = new_data;
    list->capacity = new_capacity;
}

void arraylist_push(ArrayList* list, uint32_t value) {
    if (list->size >= list->capacity) {
        arraylist_grow(list);
    }
    list->data[list->size++] = value;
}

uint32_t arraylist_pop(ArrayList* list) {
    if (list->size == 0) return 0;
    return list->data[--list->size];
}

uint32_t arraylist_get(const ArrayList* list, size_t index) {
    if (index >= list->size) return 0;
    return list->data[index];
}

void arraylist_set(ArrayList* list, size_t index, uint32_t value) {
    if (index >= list->size) return;
    list->data[index] = value;
}

size_t arraylist_size(const ArrayList* list) {
    return list->size;
}

void arraylist_clear(ArrayList* list) {
    list->size = 0;
}

void arraylist_resize(ArrayList* list, size_t new_size) {
    if (new_size > list->capacity) {
        while (list->capacity < new_size) {
            list->capacity *= 2;
        }
        uint32_t* new_data = (uint32_t*)realloc(list->data, list->capacity * sizeof(uint32_t));
        if (!new_data) return;
        list->data = new_data;
    }

    if (new_size > list->size) {
        memset(list->data + list->size, 0, (new_size - list->size) * sizeof(uint32_t));
    }

    list->size = new_size;
}

ArrayList* arraylist_clone(const ArrayList* list) {
    ArrayList* clone = arraylist_create();
    if (!clone) return NULL;

    arraylist_resize(clone, list->size);
    memcpy(clone->data, list->data, list->size * sizeof(uint32_t));

    return clone;
}
