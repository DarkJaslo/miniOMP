#include "libminiomp.h"

// Declaration of global variable for single work descriptor
miniomp_single_t miniomp_single;

extern miniomp_thread_runtime* miniomp_threads_sync;

// This routine is called when encountering a SINGLE construct. 
// Returns true if this is the thread that should execute the clause, i.e. 
// the one that first reached the construct. 

bool
GOMP_single_start (void) 
{
  miniomp_thread_runtime* runtime = miniomp_threads_sync+omp_get_thread_num();

  int single_count = runtime->single_count;
  runtime->single_count++; //add a line for clarity

  return __sync_bool_compare_and_swap(&miniomp_single.value,single_count,single_count+1);
}
