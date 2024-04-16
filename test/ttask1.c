#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>	/* OpenMP */

long result=0;

void foo() {
    #pragma omp parallel 
    {
        int argum = 1;
        #pragma omp single
        {
	    printf("I am thread %d, responsible for the generation of tasks\n", omp_get_thread_num());
	    #pragma omp task if(0)
            {
		printf("I am the first task; however I am immediately executed by the creator (%d) \n",
                       omp_get_thread_num());
            }

            #pragma omp task  shared(result) firstprivate(argum)
            {
                printf("I am the second task executed by %d\n", omp_get_thread_num());
                for (long i = 0; i < 10; i++) {
	            #pragma omp atomic
                    result += argum;
                    }
            }

            for (long i = 0; i < 10; i++) {
                argum++;
                #pragma omp task shared(result) firstprivate(argum, i)
                {
                    printf("I am instance %ld of the third task executed by %d\n", i, omp_get_thread_num());
	            #pragma omp atomic
                    result += argum;
                }
            }
        }

        #pragma omp task firstprivate(result) firstprivate(argum)
        {
            printf("Hello from the last task executed by thread %d, with result=%ld and argum = %d\n", 
	           omp_get_thread_num(), result, argum);
        }
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
