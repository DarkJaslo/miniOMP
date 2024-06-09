#include "linked_list.h"

void miniomp_linked_list_init(miniomp_linked_list_t* list)
{
    list->first = NULL;
    list->last = NULL;
    pthread_mutex_init(&list->lock, NULL);
}

void miniomp_linked_list_node_destroy(miniomp_linked_list_node_t* node)
{
  if(!node) return;
  miniomp_linked_list_node_t* next = (miniomp_linked_list_node_t*)node->next;
  if(node->deallocator)
      node->deallocator(node->data);
  free(node);
  miniomp_linked_list_node_destroy(next);
}

void miniomp_linked_list_destroy(miniomp_linked_list_t* list)
{
    pthread_mutex_destroy(&list->lock);
    miniomp_linked_list_node_destroy(list->first);
}

void miniomp_linked_list_add(miniomp_linked_list_t* list, miniomp_linked_list_node_t* node)
{
    pthread_mutex_lock(&list->lock);
    if(!list->first)
    {
        list->first = node;
        list->last = node;
    }
    else
    {
        list->last->next = node;
    }
    pthread_mutex_unlock(&list->lock);
}