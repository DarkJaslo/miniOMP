#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;

void 
GOMP_critical_start (void) {
  printf("TBI: Entering an unnamed critical, don't know if anyone else is alrady in. I proceed\n");
}

void 
GOMP_critical_end (void) {
  printf("TBI: Exiting an unnamed critical section. I can not inform anyone else, bye!\n");
}

void 
GOMP_critical_name_start (void **pptr) {
  pthread_mutex_t * plock = *pptr;
  // if plock is NULL it means that the lock associated to the name has not yet been allocated and initialized
  printf("TBI: Entering a named critical %p (%p), don't know if anyone else is alrady in. I proceed\n", pptr, plock);
}

void 
GOMP_critical_name_end (void **pptr) {
  pthread_mutex_t * plock = *pptr;
  // if plock is still NULL something went wrong
  printf("TBI: Exiting a named critical %p (%p), I can not inform anyone else, bye!\n", pptr, plock);
}

// Default barrier within a parallel region
//pthread_barrier_t miniomp_barrier;

miniomp_barrier_t miniomp_barrier;

void miniomp_barrier_init(miniomp_barrier_t* barrier, int count)
{
  barrier->threads = count;
  barrier->waiting = 0;
  barrier->exited  = 0;
  pthread_mutex_init(&barrier->mutex,NULL);
}

void miniomp_barrier_set(miniomp_barrier_t* barrier, int count)
{
  barrier->threads = count;
  barrier->waiting = 0;
  barrier->exited  = 0;
}

void miniomp_barrier_destroy(miniomp_barrier_t* barrier)
{
  pthread_mutex_destroy(&barrier->mutex);
}

void miniomp_barrier_wait(miniomp_barrier_t* barrier)
{
  printf("Entering wait\n");
  pthread_mutex_lock(&barrier->mutex);
  barrier->waiting++;
  printf("Barrier value: %d\n", barrier->waiting);
  printf("Barrier exited: %d\n", barrier->exited);
  printf("Barrier threads: %d\n", barrier->threads);
  pthread_mutex_unlock(&barrier->mutex);


  int id = (int)(long)pthread_getspecific(miniomp_specifickey);

  while (1)
  {

    //printf("Thread %d Iterate\n", id);
    __sync_synchronize();
    if(barrier->waiting >= barrier->threads)
    {
      pthread_mutex_lock(&barrier->mutex);
      if(++barrier->exited == barrier->threads)
      {
        barrier->exited = 0;
        barrier->waiting -= barrier->threads;
      }
      pthread_mutex_unlock(&barrier->mutex);
      return;
    }
  }
}

void 
GOMP_barrier() {
  miniomp_barrier_wait(&miniomp_barrier);
  //printf("TBI: Entering in barrier, but do not know how to wait for the rest. I proceed\n");
}
