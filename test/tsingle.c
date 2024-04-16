#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <omp.h>	/* OpenMP */

void foo(){
    #pragma omp parallel
    {
        for (int i = 0; i < 10; i++) {
            #pragma omp single 
            {
                printf("Hello! I'm thread %d in first loop - iteration %d\n", 
                       omp_get_thread_num(), i);
                sleep(1);
            }
        }

        for (int i = 0; i < 10; i++) {
            #pragma omp single nowait
            {
                printf("Hello! I'm thread %d in second loop - iteration %d\n", 
                       omp_get_thread_num(), i);
                sleep(1);
            }
        }
        printf("Thread %d finishing the execution of the parallel region\n", omp_get_thread_num());
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... finishing the program\n");
}
