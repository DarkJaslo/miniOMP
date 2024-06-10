#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>	/* OpenMP */

int result = 0;

void foo() {
#pragma omp parallel 
#pragma omp single
{
    printf("Hello, I am thread %d. Creating tasks...\n", omp_get_thread_num());

    #pragma omp task if(0)
    {
        printf("Hello, I am a random task\n");
    }

    #pragma omp taskgroup
    {
        //for(int j = 0; j < 2; ++j)
        #pragma omp task
        {
            printf("Hello, I am thread %d executing a 1st-level task\n",omp_get_thread_num() );
            for(int i = 0; i < 10; ++i)
            #pragma omp taskgroup
            {
                #pragma omp task
                {
                    printf("Hello, I am thread %d executing a 2nd-level task\n",omp_get_thread_num());
                    #pragma omp task
                    {
                        printf("Hello, I am thread %d executing a 3rd-level task\n",omp_get_thread_num());
                        #pragma omp task
                        {
                            printf("Hello, I am thread %d executing a 4th-level task\n",omp_get_thread_num());
                            #pragma omp task
                            {
                                printf("Hello, I am thread %d executing a 5th-level task\n",omp_get_thread_num());
                                #pragma omp atomic
                                result += 5;
                            }
                            #pragma omp atomic
                            result += 4;
                            sleep(1);
                        }
                        #pragma omp atomic
                        result += 3;
                        sleep(1);
                    }
                    #pragma omp atomic
                    result += 2;
                    sleep(1);
                }
                printf("Waiting for level 2,3,4 and 5 tasks\n");
            }
            printf("Hello, I am thread %d finishing a 1st-level task\n",omp_get_thread_num());
            #pragma omp atomic
            result += 1;
        } 
        printf("Waiting for all tasks to finish...\n");
    }

    printf("Tasks have finished, nothing else should be showing up here\n");
}
	
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main... result = %d\n", result);
}
