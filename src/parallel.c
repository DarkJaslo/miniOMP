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
  pthread_exit(NULL);
}

void
GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
  if(!num_threads) num_threads = omp_get_num_threads();
  printf("Starting a parallel region using %d threads\n", num_threads);

  for (int i=0; i<num_threads; i++)
        fn (data);
}
