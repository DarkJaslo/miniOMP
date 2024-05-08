#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;
pthread_mutex_t miniomp_named_lock;

void 
GOMP_critical_start (void) {
  //printf("TBI: Entering an unnamed critical, don't know if anyone else is alrady in. I proceed\n");
  pthread_mutex_lock(&miniomp_default_lock);
}

void 
GOMP_critical_end (void) {
  //printf("TBI: Exiting an unnamed critical section. I can not inform anyone else, bye!\n");
  pthread_mutex_unlock(&miniomp_default_lock);
}

void 
GOMP_critical_name_start (void **pptr) {
  pthread_mutex_lock(&miniomp_named_lock);

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
//printf("TBI: Entering a named critical %p (%p), don't know if anyone else is already in. I proceed\n", pptr, plock);
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

miniomp_barrier_t miniomp_barrier;
miniomp_barrier_t miniomp_parallel_barrier;

void miniomp_barrier_init(miniomp_barrier_t* barrier, int count)
{
  barrier->threads = count;
  pthread_mutex_init(&barrier->mutex,NULL);
}

void miniomp_barrier_set(miniomp_barrier_t* barrier, int count)
{
  barrier->threads = count;
}

void miniomp_barrier_destroy(miniomp_barrier_t* barrier)
{
  pthread_mutex_destroy(&barrier->mutex);
}

void miniomp_barrier_wait(miniomp_barrier_t* barrier)
{
  //We want to give one thread the task of mantaining the barrier

  //int id = (int)(long)pthread_getspecific(miniomp_specifickey);
  //printf("Thread %d entering barrier wait\n", id);

  miniomp_barrier_data* data_pointer;
  miniomp_barrier_data data; //only one thread uses it

  pthread_mutex_lock(&barrier->mutex);
  if(!barrier->data)
  {
    pthread_mutex_init(&data.mutex,NULL);
    pthread_cond_init(&data.cond,NULL);
    data.waiting = 0;
    data.done = 0;
    barrier->data = &data;
  }

  data_pointer = barrier->data; //This is not equivalent to using barrier->data, as it gets assigned NULL later

  pthread_mutex_lock(&data_pointer->mutex);
  data_pointer->waiting++;

  if(data_pointer->waiting < barrier->threads)
  {
    pthread_mutex_unlock(&barrier->mutex);
    do
    {
      pthread_cond_wait(&data_pointer->cond, &data_pointer->mutex);
    }
    while(!data_pointer->done);
  }
  else //This is the last thread
  {
    //Wake up the other threads
    data_pointer->done = 1;
    barrier->data = NULL;
    pthread_mutex_unlock(&barrier->mutex);
    pthread_cond_broadcast(&data_pointer->cond);
  }

  data_pointer->waiting--;

  if(data_pointer == &data) //Owner thread (last to exit)
  {
    while(data_pointer->waiting != 0)
    {
      pthread_cond_wait(&data_pointer->cond, &data_pointer->mutex);
    }

    pthread_mutex_unlock(&data_pointer->mutex);
    pthread_cond_destroy(&data_pointer->cond);
    pthread_mutex_destroy(&data_pointer->mutex);
  }
  else if(data_pointer->waiting == 0) //The last one, sends a signal to the owner thread
  {
    pthread_cond_signal(&data_pointer->cond);
    pthread_mutex_unlock(&data_pointer->mutex);
  }
  else{
    pthread_mutex_unlock(&data_pointer->mutex); //Any normal thread
  }
  //printf("Thread %d exiting barrier wait\n", id);
}

void 
GOMP_barrier() {
  //Assumes miniomp_barrier's value has been previously set
  miniomp_barrier_wait(&miniomp_barrier);
}
