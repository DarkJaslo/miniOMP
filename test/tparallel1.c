#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>	/* OpenMP */

int first=0, second=0, third = 0;

int foo() {
    int i, x = 1023;
    #pragma omp parallel firstprivate(x) reduction(+:first) if(x>0) 
    {
    x++; 
    first += x;
    usleep(500);
    }

    #pragma omp parallel firstprivate(x) reduction(+:second) if(0)
    {
    x++; 
    second += x;
    usleep(500);
    }

    #pragma omp parallel private(i) shared(second) reduction(+:third) num_threads(2)
    {
    third = second;
    for (i = 0; i < 16; i++)
        third++;
    usleep(500);
    }

    #pragma omp parallel private(i) reduction(+:second) 
    {
    for (i = 0; i < 16; i++)
        second++;
    usleep(500);
    }

    omp_set_num_threads(6);
    #pragma omp parallel
    {
    usleep(500);
    printf("Thread %d finished the execution of foo\n", omp_get_thread_num());
    }

    return(x);
}

int main(int argc, char *argv[]) {
    printf("Starting the execution of main program\n");
    printf("first = %d, second = %d, third = %d, x = %d\n", first, second, third, foo());
    printf("Finishing the execution of main program\n");
}
