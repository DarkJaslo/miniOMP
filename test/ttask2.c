#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sys/time.h>
double getusec_() {
        struct timeval time;
        gettimeofday(&time, NULL);
        return ((double)time.tv_sec * (double)1e6 + (double)time.tv_usec);
}

#define START_COUNT_TIME stamp = getusec_();
#define STOP_COUNT_TIME stamp = getusec_() - stamp;\
                        stamp = stamp/1e6;
#define N 1000000

double goo(int nreps) {
    double total = 0.0;
    #pragma omp parallel
    #pragma omp single
    {
        for (int rep=0; rep < nreps; rep++)
            #pragma omp task firstprivate(rep)
	    for (int i=0; i<(N/nreps); i++)
                #pragma omp atomic
	        total += sqrt(i);
    }
    return(total);
}

int main(int argc, char *argv[]) {
    double total;
    double stamp;
    START_COUNT_TIME;
    total = goo(10);
    STOP_COUNT_TIME;
    printf ("Execution time for goo(10): %0.6f with total=%0.6f\n", stamp, total);

    START_COUNT_TIME;
    total = goo(N/1000);
    STOP_COUNT_TIME;
    printf ("Execution time for goo(N/10): %0.6f with total=%0.6f\n", stamp, total);
}
