#include "libminiomp.h"

// This file implements the PARALLEL construct as part of your 
// miniomp runtime library. parallel.h contains definitions of 
// data types used here

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

// Global variable of per-thread data
extern miniomp_thread_runtime* miniomp_threads_sync;

extern miniomp_linked_list_t miniomp_task_allocations;

void
GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
  if(!num_threads) num_threads = omp_get_num_threads();

  //Save for later
  int single_count = miniomp_single.value;

  if(num_threads != miniomp_icv.parallel_threads)
  {
    miniomp_icv.parallel_threads = num_threads;
  }

  //Recreate barriers if numbers don't match
  if(num_threads != miniomp_barrier.nthreads)
  {
    miniomp_barrier_set(&miniomp_barrier,num_threads);
  }
  if(num_threads+1 != miniomp_parallel_barrier.nthreads)
  {
    miniomp_barrier_set(&miniomp_parallel_barrier, num_threads+1);
  }
  
  //Give work
  for (int i=0; i<num_threads; i++)
  {
    miniomp_thread_runtime* runtime = miniomp_threads_sync+i;
    pthread_mutex_lock(&runtime->mutex);
    runtime->fn = fn;
    runtime->data = data;
    runtime->done = 0;
    pthread_cond_signal(&runtime->do_work);
    pthread_mutex_unlock(&runtime->mutex);
  }

  // Implicit barrier
  miniomp_barrier_wait_task(&miniomp_parallel_barrier, false);

  // At least one 'single' construct happened. Some threads may have outdated
  // single_count values if they have not participated
  if(single_count != miniomp_single.value && num_threads < miniomp_icv.nthreads_var)
  {
    for(int i = num_threads; i < miniomp_icv.nthreads_var; ++i)
    {
      miniomp_thread_runtime* runtime = miniomp_threads_sync+i;
      runtime->single_count = miniomp_single.value;
    }
  }

  // Destroy tasks and task references that have been allocated during this parallel region
  miniomp_linked_list_destroy(&miniomp_task_allocations);
  miniomp_linked_list_init(&miniomp_task_allocations);
}
