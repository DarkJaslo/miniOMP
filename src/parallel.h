#include <pthread.h>

// Type declaration for parallel descriptor (arguments needed to create pthreads)
typedef struct {
    void (*fn) (void *);
    void *data;
    int id;
    // complete the definition of parallel descriptor
} miniomp_parallel_t;

extern miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
extern pthread_key_t miniomp_specifickey;

// Functions implemented in this module
void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags);
