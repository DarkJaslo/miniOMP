#include "libminiomp.h"

// Declaration of global variable for single work descriptor
miniomp_single_t miniomp_single;

pthread_key_t miniomp_single_key;

// This routine is called when encountering a SINGLE construct. 
// Returns true if this is the thread that should execute the clause, i.e. 
// the one that first reached the construct. 

// Attention:

// This relies on teams always being of the same size. When changing thread pool size,
// this should be managed so that every thread has the same value.

bool
GOMP_single_start (void) 
{
  int single_count = (int)(long)pthread_getspecific(miniomp_single_key);
  pthread_setspecific(miniomp_single_key,(void*)(long)(single_count+1));

  return __sync_bool_compare_and_swap(&miniomp_single.value,single_count,single_count+1);
}
