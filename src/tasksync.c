#include "libminiomp.h"
#include <stdlib.h>

// Called when encountering taskwait and taskgroup constructs

void 
GOMP_taskwait (void)
{
    miniomp_task_references* ref = (miniomp_task_references*)pthread_getspecific(miniomp_task_references_key);

    while(__sync_fetch_and_add(&ref->running, 0L) > 0)
    {
        try_exec_task();
    }
}

void 
GOMP_taskgroup_start (void)
{
    miniomp_task_references* ref = (miniomp_task_references*)pthread_getspecific(miniomp_taskgroup_references_key);

    miniomp_task_references* new_ref = (miniomp_task_references*)malloc(sizeof(miniomp_task_references));
    new_ref->running = 0;
    new_ref->parent = ref;

    pthread_setspecific(miniomp_taskgroup_references_key, (void*)new_ref);
}

void 
GOMP_taskgroup_end (void)
{
    // Same as taskwait, we just check a different counter
    miniomp_task_references* ref = (miniomp_task_references*)pthread_getspecific(miniomp_taskgroup_references_key);

    __sync_fetch_and_add(&ref->running, 0UL);

    while(__sync_fetch_and_add(&ref->running, 0UL) > 0)
    {
        try_exec_task();
    }

    // In this case the taskgroup reference can be freed safely
    miniomp_task_references* old_ref = ref->parent;
    pthread_setspecific(miniomp_taskgroup_references_key, (void*)old_ref);
    free(ref);
}
