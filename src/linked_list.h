#ifndef MINIOMP_LINKED_LIST
#define MINIOMP_LINKED_LIST

#include <stdlib.h>
#include "pthread.h"

// Used for temporarily storing data that cannot be freed in a comfortable way
typedef struct{
    void* next;
    void* data;
    void (*deallocator) (void*);
} miniomp_linked_list_node_t;

typedef struct{
    miniomp_linked_list_node_t* first;
    miniomp_linked_list_node_t* last;
    pthread_mutex_t lock;
} miniomp_linked_list_t;

void miniomp_linked_list_init(miniomp_linked_list_t* list);
void miniomp_linked_list_destroy(miniomp_linked_list_t* list);
void miniomp_linked_list_add(miniomp_linked_list_t* list, miniomp_linked_list_node_t* node);

#endif
