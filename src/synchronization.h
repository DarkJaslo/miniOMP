#include <pthread.h>
#include <errno.h>
// Default lock for unnamed critical sections
extern pthread_mutex_t miniomp_default_lock;
extern pthread_mutex_t miniomp_named_lock;

// Default barrier within a parallel region
//extern pthread_barrier_t miniomp_barrier;

typedef struct 
{
    pthread_mutex_t mutex;
    int             threads;
    int             waiting;
    int             exited;
} miniomp_barrier_t;

extern miniomp_barrier_t miniomp_barrier;

// Functions implemented in this module
void GOMP_critical_start (void);
void GOMP_critical_end (void);
void GOMP_critical_name_start (void **pptr);
void GOMP_critical_name_end (void **pptr);
void GOMP_barrier(void);

void miniomp_barrier_init(miniomp_barrier_t* barrier, int count);
void miniomp_barrier_set(miniomp_barrier_t* barrier, int count);
void miniomp_barrier_destroy(miniomp_barrier_t* barrier);
void miniomp_barrier_wait(miniomp_barrier_t* barrier);
