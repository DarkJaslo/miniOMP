#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>

// Maximum number of threads to be supported by our implementation
// To be used whenever you need to dimension thread-related structures
#define MAX_THREADS 32

// To implement memory barrier (flush)
//void __atomic_thread_fence(int);
#define mb() __atomic_thread_fence(3)

#include "intrinsic.h"
#include "env.h"
#include "parallel.h"
#include "synchronization.h"
#include "single.h"
#include "task.h"

// Stores all relevant data for a thread in execution
typedef struct{
  void (*fn) (void *);
  void *data;
  pthread_cond_t do_work;
  pthread_mutex_t mutex;
  unsigned int id;
  bool stop;
  bool done;
  int single_count; //keeps track of single constructs
} miniomp_thread_runtime;