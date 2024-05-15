#include <pthread.h>

// Declaration of per-thread specific key
extern pthread_key_t miniomp_specifickey;

// Functions implemented in this module
void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags);
