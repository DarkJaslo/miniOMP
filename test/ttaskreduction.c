#include <stdio.h>
#include <stdlib.h>
#include <omp.h>	/* OpenMP */

long result=1;

void foo() {
#pragma omp parallel 
#pragma omp single
    {
    #pragma omp taskgroup task_reduction(+:result) 
    for (long i = 0; i < 10000; i++) {
        #pragma omp task in_reduction(+:result) 
        result++;
    }

    #pragma omp taskgroup task_reduction(+:result)
    #pragma omp taskloop in_reduction(+:result)
    for (long i = 0; i < 10000; i++) {
        result++;
        }

    #pragma omp taskloop reduction(+:result) 
    for (long i = 0; i < 10000; i++) {
        result++;
        }

    #pragma omp task firstprivate(result)
    printf("Hello from final task, up to now result=%ld\n", result);
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
