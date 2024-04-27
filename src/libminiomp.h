#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

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

typedef struct{
  void (*fn) (void *);
  void *data;
  pthread_cond_t do_work;
  pthread_mutex_t mutex;
  unsigned int id;
  bool stop;
  bool done;
} miniomp_thread_runtime;