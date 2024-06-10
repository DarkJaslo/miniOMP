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
    //A partir d'ara, les tasques que es creen tenen aquesta referencia com pare de taskgroup
    miniomp_task_references* ref = (miniomp_task_references*)pthread_getspecific(miniomp_task_references_key);

    //El que se m'acut per fer una assignació atòmica
    __sync_bool_compare_and_swap(&ref->parent_taskgroup_running, ref->parent_taskgroup_running,&ref->taskgroup_running );
}

void 
GOMP_taskgroup_end (void)
{
    //Esperem a totes les tasques del grup, igual que a taskwait, només que es mira un altre comptador.
    miniomp_task_references* ref = (miniomp_task_references*)pthread_getspecific(miniomp_task_references_key);

    while(__sync_fetch_and_add(&ref->taskgroup_running, 0L) > 0)
    {
        try_exec_task();
    }
}
