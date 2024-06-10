#include "linked_list.h"

typedef struct {
    void* parent;
    unsigned long running;
    unsigned long taskgroup_running;
    unsigned long* parent_taskgroup_running;
} miniomp_task_references;

/* This structure describes a "task" to be run by a thread.  */
typedef struct {
    void (*fn)(void *);
    void (*data);
    miniomp_task_references* ref;
} miniomp_task_t;

#define MAXELEMENTS_TQ 2048

typedef struct {
    miniomp_task_t * queue[MAXELEMENTS_TQ];
    int head;
    int tail;
    int num_elems;
    pthread_mutex_t lock_taskqueue;
    volatile int in_execution;
    // complete with additional fields if needed or replace previous ones
} miniomp_taskqueue_t;

extern miniomp_taskqueue_t miniomp_taskqueue;
extern pthread_key_t miniomp_task_references_key;
extern miniomp_linked_list_t miniomp_task_allocations;

void store_ref_in_list(miniomp_task_references* ref, miniomp_linked_list_t* list);

// funtions to implement basic management operations on taskqueue
void TQinit(miniomp_taskqueue_t *task_queue);
void TQdestroy(miniomp_taskqueue_t *task_queue);
bool TQis_empty(miniomp_taskqueue_t *task_queue);
bool TQis_full(miniomp_taskqueue_t *task_queue);
void TQenqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t *task_descriptor); 
miniomp_task_t * TQdequeue(miniomp_taskqueue_t *task_queue);
int  TQin_execution(miniomp_taskqueue_t* task_queue);

// Functions implemented in task* modules
void GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, bool if_clause, unsigned flags,
           void **depend, int priority);
void try_exec_task();
void exec_task(miniomp_task_t* task);
void exec_task_now(void (*fn)(void *), void (*data));

void GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
               long arg_size, long arg_align, unsigned flags,
               unsigned long num_tasks, int priority,
               long start, long end, long step);
extern int miniomp_intaskgroup; 
void GOMP_taskgroup_start (void);
void GOMP_taskgroup_end (void);
void GOMP_taskgroup_reduction_register (uintptr_t *data);
void GOMP_taskgroup_reduction_unregister (uintptr_t *data);
void GOMP_task_reduction_remap (size_t cnt, size_t cntorig, void **ptrs);
