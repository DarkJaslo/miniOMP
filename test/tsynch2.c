#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "omp.h"

#define N     100000 //Original had 1.000.000 -> takes too long
#define NREPS 10

volatile double result=0.0, result_even=0.0, result_odd=0.0;

void foo1(long n) {
    for (long i = 1; i <= n; i++) {
        if (i%2) {
            #pragma omp critical(even)
            result_even += 1;
            }
        else {
            #pragma omp critical(odd)
            result_odd += 1;
            }
    }
}

double foo2(long n) {
    double result = 0.0;
    for (long i = 0; i < n; i++) {
	result++;
    	#pragma omp barrier
    }
    return(result);
}

void foo3(long n) {
    for (long i = 0; i < n; i++) {
    	#pragma omp critical
    	result += (result_even + result_odd);
    	}
}

int main(int argc, char *argv[]) {
    double stamp1=0.0, stamp2=0.0, stamp3=0.0;
    double total=0.0;

    int nthreads = omp_get_max_threads();
    #pragma omp parallel num_threads(nthreads) reduction(+:stamp1, stamp2, stamp3, total) 
    {
    for (int rep=0; rep < NREPS; rep++) {
	stamp1 -= omp_get_wtime();
    	foo1(N/NREPS);
	stamp1 += omp_get_wtime();
	stamp2 -= omp_get_wtime();
    	total += foo2(N/NREPS);
	stamp2 += omp_get_wtime();
	stamp3 -= omp_get_wtime();
    	foo3(N/NREPS);
	stamp3 += omp_get_wtime();
	stamp2 -= omp_get_wtime();
    	total += foo2(N/NREPS);
	stamp2 += omp_get_wtime();
    }
    }
    printf ("Execution time with %d threads:\n  - named critical: %lf\n  - unnamed critical: %lf\n  - barrier: %lf\nCheck value=%lf\n", nthreads, stamp1/nthreads, stamp3/nthreads, stamp2/(2*nthreads), result+total);
}
