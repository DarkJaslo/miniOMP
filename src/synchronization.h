#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

// Default lock for unnamed critical sections
extern pthread_mutex_t miniomp_default_lock;
extern pthread_mutex_t miniomp_named_lock;

//To be used in normal GOMP_barrier()
extern pthread_barrier_t miniomp_barrier;
extern int               miniomp_barrier_count;

//To be specifically used in parallel regions so that an additional thread waits for it
extern pthread_barrier_t miniomp_parallel_barrier;
extern int               miniomp_parallel_barrier_count;

// Functions implemented in this module
void GOMP_critical_start (void);
void GOMP_critical_end (void);
void GOMP_critical_name_start (void **pptr);
void GOMP_critical_name_end (void **pptr);
void GOMP_barrier(void);