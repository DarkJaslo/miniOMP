#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "omp.h"

#define N 10000000

double goo(int nreps) {
    double total = 0.0;
    for (int rep=0; rep < nreps; rep++)
        #pragma omp parallel reduction(+: total)
	for (int i=0; i<(N/nreps); i++)
	    total += sqrt(i);
    return(total);
}

int main(int argc, char *argv[]) {
    double total;

    double stamp = -omp_get_wtime();
    total = goo(N/100000);
    stamp += omp_get_wtime();
    printf ("Execution time for goo(N/100000): %lf with total=%lf\n", stamp, total);

    stamp = -omp_get_wtime();
    total = goo(N/10000);
    stamp += omp_get_wtime();
    printf ("Execution time for goo(N/10000): %lf with total=%lf\n", stamp, total);

    stamp = -omp_get_wtime();
    total = goo(N/1000);
    stamp += omp_get_wtime();
    printf ("Execution time for goo(N/1000): %lf with total=%lf\n", stamp, total);
}
