#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;
// Default lock for named critical sections
pthread_mutex_t miniomp_named_lock;

miniomp_linked_list_t named_criticals;

void miniomp_barrier_init(miniomp_barrier_t* barrier, int num_threads)
{
  barrier->nthreads = num_threads;
  barrier->reset = 0; //equivalent to { barrier->sequence = 0; barrier->count = 0; }, but at the same time
}

void miniomp_barrier_set(miniomp_barrier_t* barrier, int num_threads)
{
  barrier->nthreads = num_threads;
}

void miniomp_barrier_destroy(miniomp_barrier_t* barrier)
{
  //No need to do anything here (yet).
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
        sched_yield(); //GREATLY improves performance as it reduces simultaneous locks on sequence
      }
      break;
    }
    //Case 2: We are the last thread of the batch
    if(count == barrier->nthreads)
    {
      //In one step, increment the sequence and reset to zero the count
      //this is possible because reset is in a union with count and sequence -> they share memory
      mb();
      barrier->reset = barrier->sequence+1;
      mb();
      break;
    }

    //Case 3: we arrived late for the current batch, wait for the next
    while((__sync_fetch_and_add(&barrier->sequence,0)) == seq)
    {
      sched_yield(); //GREATLY improves performance as it reduces simultaneous locks on sequence
    }
  }
}

void miniomp_barrier_wait_task(miniomp_barrier_t* barrier, bool worker)
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
        if(worker)
          try_exec_task();
        sched_yield(); //GREATLY improves performance as it reduces simultaneous locks on sequence
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
      sched_yield(); //GREATLY improves performance as it reduces simultaneous locks on sequence
    }
  }

  // While there are active tasks
  while(!TQis_empty(&miniomp_taskqueue) || TQin_execution(&miniomp_taskqueue) > 0)
  {
    if(worker)
      try_exec_task();
    else
      sched_yield();
  }
}

// Custom deallocator for named criticals
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
    *pptr = (miniomp_named_critical_t*)malloc(sizeof(miniomp_named_critical_t));
    critical = *pptr;
    pthread_mutex_init(&critical->mutex, NULL);

    //Linked list node creation
    miniomp_linked_list_node_t* node = (miniomp_linked_list_node_t*)malloc(sizeof(miniomp_linked_list_node_t));
    node->next = NULL;
    node->data = (void*)critical;
    node->deallocator = miniomp_critical_node_destroy;

    miniomp_linked_list_add(&named_criticals,node);
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

miniomp_barrier_t miniomp_barrier;
miniomp_barrier_t miniomp_parallel_barrier;

void 
GOMP_barrier() 
{
  miniomp_barrier_wait(&miniomp_barrier);
}
