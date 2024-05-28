#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;
// Default lock for named critical sections
pthread_mutex_t miniomp_named_lock;

miniomp_linked_list_t named_criticals;

void miniomp_barrier_init(miniomp_barrier_t* barrier, int num_threads)
{
  barrier->threads = num_threads;
  barrier->data = NULL;
  pthread_mutex_init(&barrier->mutex,NULL);
}

void miniomp_barrier_set(miniomp_barrier_t* barrier, int num_threads)
{
  barrier->threads = num_threads;
}

void miniomp_barrier_destroy(miniomp_barrier_t* barrier)
{
  pthread_mutex_destroy(&barrier->mutex);
}

void miniomp_barrier_wait(miniomp_barrier_t* barrier)
{
  //We want to give one thread the task of mantaining the barrier

  //int id = (int)(long)pthread_getspecific(miniomp_specifickey);
  //printf("Thread %d entering wait\n", id);

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

  data_pointer = barrier->data; //This is not equivalent to using barrier->data from now on, as it gets assigned NULL later

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

  if(data_pointer == &data) //Owner thread (has to be the last to exit)
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
  //printf("Thread %d exiting wait\n", id);
}


// TODO: make it work

/*
  Idea: after all threads have passed the first phase, keep looking for tasks while:

  1. There are tasks
  2. There are threads which execute tasks
    - Ensures that tasks that create tasks are executed

  Then, proceed with the deallocation phase
*/

// While waiting, checks task pool and executes tasks
void miniomp_barrier_wait_task(miniomp_barrier_t* barrier/*, miniomp_taskqueue_t* queue*/)
{
  //We want to give one thread the task of mantaining the barrier

  //int id = (int)(long)pthread_getspecific(miniomp_specifickey);
  //printf("Thread %d entering wait\n", id);

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

  data_pointer = barrier->data; //This is not equivalent to using barrier->data from now on, as it gets assigned NULL later

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

  ////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////

  //                  Check for tasks here. Beware of mutexes and who has what

  ////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////



  data_pointer->waiting--;

  if(data_pointer == &data) //Owner thread (has to be the last to exit)
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
  //printf("Thread %d exiting wait\n", id);
}

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

//pthread_barrier_t miniomp_barrier;
miniomp_barrier_t miniomp_barrier;
//int               miniomp_barrier_count;
//pthread_barrier_t miniomp_parallel_barrier;
miniomp_barrier_t miniomp_parallel_barrier;
//int               miniomp_parallel_barrier_count;

void 
GOMP_barrier() {
  miniomp_barrier_wait(&miniomp_barrier);
  //pthread_barrier_wait(&miniomp_barrier);
}
