#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "linked_list.h"

// Default lock for unnamed critical sections
extern pthread_mutex_t miniomp_default_lock;
// Lock for named critical sections
extern pthread_mutex_t miniomp_named_lock;

// Barrier implementation
typedef struct
{
    union
    {
        struct
        {
            unsigned int sequence;
            unsigned int count;
        };
        unsigned long long reset;
    };
    unsigned int nthreads;
} miniomp_barrier_t;

typedef struct{
    pthread_mutex_t mutex;
} miniomp_named_critical_t;

void miniomp_critical_node_destroy(void* data);
void miniomp_barrier_init(miniomp_barrier_t* barrier, int num_threads);
void miniomp_barrier_set(miniomp_barrier_t* barrier, int num_threads);
void miniomp_barrier_destroy(miniomp_barrier_t* barrier);
void miniomp_barrier_wait(miniomp_barrier_t* barrier);

// While waiting, checks task pool and executes tasks if worker == TRUE
void miniomp_barrier_wait_task(miniomp_barrier_t* barrier, bool worker);

extern miniomp_linked_list_t named_criticals;

//To be used in normal GOMP_barrier()
extern miniomp_barrier_t miniomp_barrier;

//To be specifically used in parallel regions so that the additional thread waits for it
extern miniomp_barrier_t miniomp_parallel_barrier;

// Functions implemented in this module
void GOMP_critical_start (void);
void GOMP_critical_end (void);
void GOMP_critical_name_start (void **pptr);
void GOMP_critical_name_end (void **pptr);
void GOMP_barrier(void);