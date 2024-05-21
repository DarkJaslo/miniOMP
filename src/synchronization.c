#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;
// Default lock for named critical sections
pthread_mutex_t miniomp_named_lock;

miniomp_linked_list_t named_criticals;

void miniomp_linked_list_destroy(miniomp_linked_list_node_t* first)
{
  if(!first) return;
  miniomp_linked_list_node_t* next = first->next;
  first->deallocator(first->data);
  free(first);
  miniomp_linked_list_destroy(next);
}

void miniomp_critical_node_destroy(void* data)
{
  miniomp_named_critical_t* critical = (miniomp_named_critical_t*)data;
  pthread_mutex_destroy(&critical->mutex);
}

void 
GOMP_critical_start (void) 
{
  pthread_mutex_lock(&miniomp_default_lock);
}

void 
GOMP_critical_end (void) 
{
  pthread_mutex_unlock(&miniomp_default_lock);
}

void 
GOMP_critical_name_start (void **pptr) {
  pthread_mutex_lock(&miniomp_named_lock); //To ensure only one thread allocates the mutex
  miniomp_named_critical_t * critical = *pptr;

  // if critical is NULL it means that the lock associated to the name has not yet been allocated and initialized
  if(!critical)
  {
    //printf("critical is null\n");
    *pptr = (miniomp_named_critical_t*)malloc(sizeof(miniomp_named_critical_t));
    critical = *pptr;
    pthread_mutex_init(&critical->mutex, NULL);

    //Linked list node creation
    miniomp_linked_list_node_t* node = (miniomp_linked_list_node_t*)malloc(sizeof(miniomp_linked_list_node_t));
    node->next = NULL;
    node->data = (void*)critical;
    node->deallocator = miniomp_critical_node_destroy;
    if(!named_criticals.first)
    {
      named_criticals.first = node;
      named_criticals.last = node;
    }
  }
  pthread_mutex_lock(&critical->mutex);
  pthread_mutex_unlock(&miniomp_named_lock);
}

void 
GOMP_critical_name_end (void **pptr) 
{
  miniomp_named_critical_t * critical = *pptr;
  pthread_mutex_unlock(&critical->mutex);
}

pthread_barrier_t miniomp_barrier;
int               miniomp_barrier_count;
pthread_barrier_t miniomp_parallel_barrier;
int               miniomp_parallel_barrier_count;

void 
GOMP_barrier() {
  pthread_barrier_wait(&miniomp_barrier);
}
