#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;
// Default lock for named critical sections
pthread_mutex_t miniomp_named_lock;

miniomp_linked_list_t named_criticals;

void miniomp_barrier_init(miniomp_barrier_t* barrier, int num_threads)
{
  barrier->nthreads = num_threads;
  barrier->reset = 0; //equivalent to { barrier->sequence = 0; barrier->count = 0; } 
}

void miniomp_barrier_set(miniomp_barrier_t* barrier, int num_threads)
{
  barrier->nthreads = num_threads;
}

void miniomp_barrier_destroy(miniomp_barrier_t* barrier)
{
  //No need to do anything as of now.
}

void miniomp_barrier_wait(miniomp_barrier_t* barrier)
{
  while(1)
  {
    unsigned int seq = __sync_fetch_and_add(&barrier->sequence,0);
    unsigned int count = __sync_add_and_fetch(&barrier->count,1);

    //Case 1: Wait for more threads
    if(count < barrier->nthreads)
    {
      while((__sync_fetch_and_add(&barrier->sequence,0)) == seq) //Wait for the sequence to be increased
      {
        sched_yield(); //GREATLY improves performance as it reduces locks on sequence
      }
      break;
    }
    //Case 2: We are the last thread of the batch
    if(count == barrier->nthreads)
    {
      //In one step, increment the sequence and reset to zero the count
      //this is possible because reset is in an union with count and sequence -> they share memory
      mb();
      barrier->reset = barrier->sequence+1;
      mb();
      break;
    }

    //Case 3: we arrived late for the current batch, wait for the next
    while((__sync_fetch_and_add(&barrier->sequence,0)) == seq)
    {
      sched_yield(); //GREATLY improves performance as it reduces locks on sequence
    }
  }
}

// TODO: make it work
/*
  Idea: after all threads have passed the first phase, keep looking for tasks while:

  1. There are tasks
  2. There are threads which execute tasks
    - Ensures that tasks that create tasks are executed

  Then, proceed with the deallocation phase
*/
void miniomp_barrier_wait_task(miniomp_barrier_t* barrier)
{
  while(1)
  {
    unsigned int seq = __sync_fetch_and_add(&barrier->sequence,0);
    unsigned int count = __sync_add_and_fetch(&barrier->count,1);

    //Case 1: Wait for more threads
    if(count < barrier->nthreads)
    {
      while((__sync_fetch_and_add(&barrier->sequence,0)) == seq) //Wait for the sequence to be increased
      {
        /* Look for tasks */
        //exec_task();
        sched_yield(); //GREATLY improves performance as it reduces locks on sequence
      }
      break;
    }
    //Case 2: We are the last thread of the batch
    if(count == barrier->nthreads)
    {
      //In one step, increment the sequence and reset to zero the count
      //this is possible because reset is in an union with count and sequence -> they share memory
      mb();
      barrier->reset = barrier->sequence+1;
      mb();
      break;
    }

    //Case 3: we arrived late for the current batch, wait for the next
    while((__sync_fetch_and_add(&barrier->sequence,0)) == seq)
    {
      //exec_task();
      sched_yield(); //GREATLY improves performance as it reduces locks on sequence
    }
  }
}

void exec_task()
{
  pthread_mutex_lock(&miniomp_taskqueue.lock_taskqueue);
  if(!TQis_empty(&miniomp_taskqueue))
  {
    // Grab a task
    miniomp_task_t* task = TQdequeue(&miniomp_taskqueue);
    pthread_mutex_unlock(&miniomp_taskqueue);
    task->fn(task->data);
  }
  else
  {
    pthread_mutex_unlock(&miniomp_taskqueue);
  }
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
