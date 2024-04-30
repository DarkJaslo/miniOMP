#include "libminiomp.h"

// This file implements the PARALLEL construct as part of your 
// miniomp runtime library. parallel.h contains definitions of 
// data types used here

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

extern miniomp_thread_runtime* miniomp_threads_sync;
extern volatile int miniomp_active_threads;

void
GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
  if(!num_threads) num_threads = omp_get_num_threads();
printf("Starting a parallel region using %d threads\n", num_threads);

  miniomp_barrier_set(&miniomp_barrier, num_threads+1);

  //__sync_add_and_fetch(&miniomp_active_threads,(int)num_threads);
  __sync_synchronize();
  
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

  #if 1
  #elif 0
  //printf("Active threads: %d\n", miniomp_active_threads);
  while(miniomp_active_threads > 0)
  {
    //printf("Active threads: %d\n", miniomp_active_threads);
    //usleep(10000);
  }
  //printf("Active threads 2: %d\n", miniomp_active_threads);
  
  #else

  int done_threads[num_threads];
  int num_done_threads = 0;
  for(int i = 0; i < num_threads; ++i)
    done_threads[i] = 0;

  int t = 0;
  
  while (num_done_threads < num_threads)
  {
    if(done_threads[t])
    {
      if(++t>=num_threads) t = 0;
      continue;
    }

    miniomp_thread_runtime* runtime = miniomp_threads_sync+t;
    pthread_mutex_lock(&runtime->mutex);
    if(runtime->fn == NULL)
    {
      //Thread is done
      done_threads[t] = 1;
      num_done_threads++;
    } 
    if(++t>=num_threads) t = 0;
    pthread_mutex_unlock(&runtime->mutex);
  }

  #endif
printf("Parallel pre-wait\n");
  miniomp_barrier_wait(&miniomp_barrier);
printf("Ending parallel region\n");
}
