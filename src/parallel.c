#include "libminiomp.h"

// This file implements the PARALLEL construct as part of your 
// miniomp runtime library. parallel.h contains definitions of 
// data types used here

// Declaration of array for storing pthread identifier from 
// pthread_create function
pthread_t *miniomp_threads;

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

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

  miniomp_threads = (pthread_t*)malloc(num_threads*sizeof(pthread_t));
  miniomp_parallel = (miniomp_parallel_t*)malloc(num_threads*sizeof(miniomp_parallel_t));

  for (int i=0; i<num_threads; i++)
  {
    miniomp_parallel[i].id=i;
    miniomp_parallel[i].fn=fn;
    miniomp_parallel[i].data=data;
    pthread_create(&miniomp_threads[i],NULL,worker,miniomp_parallel+i);
  }

  for (int i=0; i<num_threads; i++)
  {
    pthread_join(miniomp_threads[i],NULL);
  }


  free(miniomp_threads);
}
