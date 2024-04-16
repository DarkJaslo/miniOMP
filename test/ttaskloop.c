#include <stdio.h>
#include <stdlib.h>
#include <omp.h>	/* OpenMP */

long result=0;

void foo() {
#pragma omp parallel 
#pragma omp single
    {
    #pragma omp task shared(result) 
    for (long i = 0; i < 10; i++) {
        printf("Iteration %ld in first loop executed by thread %d\n", i, omp_get_thread_num());
	#pragma omp atomic
        result++;
        }

    #pragma omp taskloop shared(result) nogroup
    for (long i = 0; i < 10; i++) {
        printf("Iteration %ld in second loop executed by thread %d\n", i, omp_get_thread_num());
	#pragma omp atomic
        result++;
        }

    #pragma omp taskloop shared(result) num_tasks(2) 
    for (long i = 0; i < 10; i++) {
        printf("Iteration %ld in third loop executed by thread %d\n", i, omp_get_thread_num());
	#pragma omp atomic
        result++;
        }

    #pragma omp taskwait

    #pragma omp taskloop shared(result) grainsize(3)
    for (long i = 9; i >= 0; i--) {
        printf("Iteration %ld in fourth loop executed by thread %d\n", i, omp_get_thread_num());
	#pragma omp atomic
        result++;
        }

    #pragma omp task firstprivate(result)
    printf("Hello from final task, up to now result = %ld\n", result);
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
