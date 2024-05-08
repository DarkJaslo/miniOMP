#include "libminiomp.h"

// This file implements the PARALLEL construct as part of your 
// miniomp runtime library. parallel.h contains definitions of 
// data types used here

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

extern miniomp_thread_runtime* miniomp_threads_sync;

void
GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
  if(!num_threads) num_threads = omp_get_num_threads();
//printf("Starting a parallel region using %d threads\n", num_threads);

  //We set the number of threads the barrier at the end of parallel needs to wait for
  miniomp_barrier_set(&miniomp_barrier, num_threads+1);
  
  for (int i=0; i<num_threads; i++)
  {
    miniomp_thread_runtime* runtime = miniomp_threads_sync+i;
    pthread_mutex_lock(&runtime->mutex);
    runtime->fn = fn;
    runtime->data = data;
    runtime->done = 0;
  //printf("Signaling thread %d\n", i);
    pthread_cond_signal(&runtime->do_work);
    pthread_mutex_unlock(&runtime->mutex);
  }

  GOMP_barrier();
//printf("Ending parallel region\n");
}
