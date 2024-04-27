#include "libminiomp.h"

// This file implements the PARALLEL construct as part of your 
// miniomp runtime library. parallel.h contains definitions of 
// data types used here

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Global semaphore for correct parallel termination
sem_t miniomp_parallel_semaphore;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

extern miniomp_thread_runtime* miniomp_threads_sync;

// This is the prototype for the Pthreads starting function
void *
worker(void *args) {
  // insert all necessary code here for:
  //   1) save thread-specific data
  //   2) invoke the per-threads instance of function encapsulating the parallel region
  //   3) exit the function
  miniomp_parallel_t* tmp_info = (miniomp_parallel_t*)args;
  pthread_setspecific(miniomp_specifickey,(void*)(long)tmp_info->id);
  tmp_info->fn(tmp_info->data);
  pthread_exit(NULL);
}

void
GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
  if(!num_threads) num_threads = omp_get_num_threads();
printf("Starting a parallel region using %d threads\n", num_threads);

  sem_init(&miniomp_parallel_semaphore,0,-1*num_threads+1);

  for (int i=0; i<num_threads; i++)
  {
    miniomp_thread_runtime* runtime = miniomp_threads_sync+i;
    pthread_mutex_lock(&runtime->mutex);
    runtime->fn = fn;
    runtime->data = data;
    runtime->done = 0;
  printf("Signaling thread %d\n", i);
    pthread_cond_signal(&runtime->do_work);
    pthread_mutex_unlock(&runtime->mutex);
  }

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

    /* 
    printf("OK state: ");
    for(int k = 0; k < num_threads; ++k)
    {
      if(done_threads[k])
      {
        printf("%d-Y ",k);
      }
      else
      {
        printf("%d-N ",k);
      }
    }
    printf("\n");
    */

    miniomp_thread_runtime* runtime = miniomp_threads_sync+t;
    pthread_mutex_lock(&runtime->mutex);
    if(runtime->done)
    {
      //Thread is done
      done_threads[t] = 1;
      num_done_threads++;
    } 
    if(++t>=num_threads) t = 0;
    pthread_mutex_unlock(&runtime->mutex);
  }
printf("Ending parallel region\n");

  //Wait for threads to be waiting here
  sem_wait(&miniomp_parallel_semaphore);
}
