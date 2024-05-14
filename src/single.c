#include "libminiomp.h"

// Declaratiuon of global variable for single work descriptor
miniomp_single_t miniomp_single;

// This routine is called when encountering a SINGLE construct. 
// Returns true if this is the thread that should execute the clause, i.e. 
// the one that first reached the construct. 

bool
GOMP_single_start (void) 
{
  if(__sync_bool_compare_and_swap(&miniomp_single.value,0,1))
  {
    GOMP_barrier();
    miniomp_single.value = 0;
    return 1;
  }
  else
  {
    GOMP_barrier();
    return 0;
  }
}
