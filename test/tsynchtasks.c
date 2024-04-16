#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>	/* OpenMP */

long result=0, first, second;

void foo() {
#pragma omp parallel 
#pragma omp single
    {
    #pragma omp task
        {
	printf("Hello, I am the first task executed by thread %d going to sleep(6)\n", omp_get_thread_num());
        sleep(6);
        first = 6;
	printf("Hello again ... first task executed by thread %d finished\n", omp_get_thread_num());
        }

    #pragma omp task
        {
	printf("Hello, I am the second task executed by thread %d going to sleep(3)\n", omp_get_thread_num());
        sleep(3);
        second = 3;
	printf("Hello again ... second task executed by thread %d finished\n", omp_get_thread_num());
        }

    int argum = 1;
    #pragma omp taskgroup
    {
    #pragma omp task  shared(result) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result += argum;
        }

    argum++;
    #pragma omp task shared(result) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result += argum;
        }

    printf("Here I am waiting only for the termination of the two computation loops ...\n");
    }

    argum = result;

    for (long i = 0; i < 10; i++) {
        #pragma omp task shared(result) firstprivate(argum)
	#pragma omp atomic
        result += argum;
        }

    printf("... and now waiting for all remaining tasks to finish with result = %ld and argum = %d\n", result, argum);
    #pragma omp taskwait
    printf("All tasks finished!\n");

    result += first + second;

    #pragma omp task firstprivate(result) firstprivate(argum)
    printf("Hello from third task, up to now result=%ld and argum = %d\n", result, argum);
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
