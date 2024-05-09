#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;
// Default lock for named critical sections
pthread_mutex_t miniomp_named_lock;

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
  pthread_mutex_t * plock = *pptr;

  // if plock is NULL it means that the lock associated to the name has not yet been allocated and initialized
  if(!plock)
  {
    //printf("plock is null\n");
    *pptr = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    plock = *pptr;
    pthread_mutex_init(plock, NULL);
  }
  pthread_mutex_unlock(&miniomp_named_lock);
  pthread_mutex_lock(plock);
}

// TODO: free plock
void 
GOMP_critical_name_end (void **pptr) 
{
  pthread_mutex_t * plock = *pptr;
  pthread_mutex_unlock(plock);

  //if(plock)
  //{
  //  pthread_mutex_destroy(plock);
  //  free(plock);
  //}
}

pthread_barrier_t miniomp_barrier;
int               miniomp_barrier_count;
pthread_barrier_t miniomp_parallel_barrier;
int               miniomp_parallel_barrier_count;

void 
GOMP_barrier() {
  pthread_barrier_wait(&miniomp_barrier);
}
