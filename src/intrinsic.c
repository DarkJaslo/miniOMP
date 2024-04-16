#include "libminiomp.h"

void omp_set_num_threads (int n) {
    miniomp_icv.nthreads_var = (n > 0 ? n : 1);
}

int omp_get_num_threads (void) {
    return(miniomp_icv.nthreads_var);
}

// Warning: to be changed if the definition of miniomp_specifickey is changed
int omp_get_thread_num (void) {
    return((int)(long)pthread_getspecific(miniomp_specifickey));
}

// No need to implement this function, it is just involked by Extrae at some point
// and returns the current nesting for parallel regions
int omp_get_level (void) {
    return(1);
}
