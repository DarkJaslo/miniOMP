#include "libminiomp.h"
#include <stdlib.h>

// Called when encountering taskwait and taskgroup constructs

int miniomp_intaskgroup;

void 
GOMP_taskwait (void)
{
    if (!miniomp_intaskgroup) // just printed if function is not invoked from fake taskgroup implementation
        printf("TBI: Entered in taskwait, there should be no pending tasks, so I proceed\n");
}


void 
GOMP_taskgroup_start (void)
{
    miniomp_intaskgroup = 1;
    printf("TBI: Entering a taskgroup region, at the end of which I should wait for tasks created here\n");
}

void 
GOMP_taskgroup_end (void)
{
    if (miniomp_intaskgroup) GOMP_taskwait();
    miniomp_intaskgroup = 0;
    printf("TBI: Finished a taskgroup region, there should be no pending tasks, so I proceed\n");
}
