#include "libminiomp.h"

miniomp_taskqueue_t miniomp_taskqueue;
miniomp_linked_list_t miniomp_task_allocations;
pthread_key_t miniomp_task_references_key;

// Initializes the task queue
void TQinit(miniomp_taskqueue_t *task_queue) {
    task_queue->num_elems = 0;
    task_queue->head = 0;
    task_queue->tail = 0;
    task_queue->in_execution = 0;
    pthread_mutex_init (&task_queue->lock_taskqueue, NULL);
    return;
}

void TQdestroy(miniomp_taskqueue_t *task_queue)
{
    pthread_mutex_destroy(&task_queue->lock_taskqueue);
}

// Checks if the task queue is empty
bool TQis_empty(miniomp_taskqueue_t *task_queue) {

    return __sync_fetch_and_add(&task_queue->num_elems,0) == 0;
    //bool result = (task_queue->num_elems==0);
    //return result;
}

// Checks if the task queue is full
bool TQis_full(miniomp_taskqueue_t *task_queue) {
    return __sync_fetch_and_add(&task_queue->num_elems,0) == MAXELEMENTS_TQ;
    //bool result = (task_queue->num_elems==MAXELEMENTS_TQ);
    //return result;
}

// Enqueues the task descriptor at the tail of the task queue
void TQenqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t *task_descriptor) {
    task_queue->queue[task_queue->tail] = task_descriptor;
    task_queue->tail++;
    if (task_queue->tail == MAXELEMENTS_TQ)
        task_queue->tail = 0;
    __sync_fetch_and_add(&task_queue->num_elems,1);
}

// Dequeue the task descriptor at the head of the task queue
miniomp_task_t * TQdequeue(miniomp_taskqueue_t *task_queue) { 
    miniomp_task_t *out = task_queue->queue[task_queue->head];
    task_queue->head++;
    if (task_queue->head == MAXELEMENTS_TQ)
        task_queue->head = 0;
    __sync_fetch_and_add(&task_queue->num_elems,-1);
    return(out);
}

int TQin_execution(miniomp_taskqueue_t* task_queue)
{
    return __sync_fetch_and_add(&task_queue->in_execution,0);
}

#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)
#define GOMP_TASK_FLAG_REDUCTION        (1 << 12)

// Called when encountering an explicit task directive. Arguments are:
//      1. void (*fn) (void *): the generated outlined function for the task body
//      2. void *data: the parameters for the outlined function
//      3. void (*cpyfn) (void *, void *): copy function to replace the default memcpy() from 
//                                         function data to each task's data
//      4. long arg_size: specify the size of data
//      5. long arg_align: alignment of the data
//      6. bool if_clause: the value of if_clause. true --> 1, false -->0; default is set to 1 by compiler
//      7. unsigned flags: see #define list above

void
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, bool if_clause, unsigned flags,
           void **depend, int priority)
{
    //printf("Thread %d encountered a task\n",omp_get_thread_num());

    //if (!if_clause) printf(" with if clause set to 0, I am executing it immediately!\n");
    //else printf(", I am executing it immediately until you implement me!\n");

    // This part of the code appropriately copies data to be passed to task function,
    // either using a compiler cpyfn function or just memcopy otherwise; no need to
    // fully understand it for the purposes of this assignment
    char *arg;
    if (__builtin_expect (cpyfn != NULL, 0)) {
	  char *buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          arg       = (char *) (((uintptr_t) buf + arg_align - 1)
                               & ~(uintptr_t) (arg_align - 1));
          cpyfn (arg, data);
    } else {
          arg       =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          memcpy (arg, data, arg_size);
    }

    if(!if_clause) //if clause set to 0, execute immediately
    {
        exec_task_now(fn,arg);
    }
    else
    {
        pthread_mutex_lock(&miniomp_taskqueue.lock_taskqueue);
        if(TQis_full(&miniomp_taskqueue))
        {
            pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue); 
            exec_task_now(fn,arg);
        }
        else
        {
            // Task can be inserted, create it
            miniomp_task_t* task = (miniomp_task_t*)malloc(sizeof(miniomp_task_t));
            task->data = (void*)arg;
            task->fn = fn;

            //Create task reference and increase the counter of my reference ("I have a new child task")
            miniomp_task_references* ref = (miniomp_task_references*)pthread_getspecific(miniomp_task_references_key);
            miniomp_task_references* new_ref = (miniomp_task_references*)malloc(sizeof(miniomp_task_references));
            new_ref->parent = ref;
            new_ref->running = 0;
            new_ref->taskgroup_running = 0;
            if(ref->parent_taskgroup_running)
            {
                new_ref->parent_taskgroup_running = ref->parent_taskgroup_running;
            }
            else 
            {
                new_ref->parent_taskgroup_running = NULL;
            }

            task->ref = new_ref;
            store_ref_in_list(new_ref,&miniomp_task_allocations); //to free later
            __sync_fetch_and_add(&ref->running,1UL);
            if(new_ref->parent_taskgroup_running)
                __sync_fetch_and_add(new_ref->parent_taskgroup_running,1UL);
            TQenqueue(&miniomp_taskqueue,task);
            pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue); 
        } 
    }  
    //printf("Task by thread %d is done enqueuing!\n", omp_get_thread_num());
}

void try_exec_task()
{
  pthread_mutex_lock(&miniomp_taskqueue.lock_taskqueue);

  if(!TQis_empty(&miniomp_taskqueue))
  {
    // Grab a task
    //printf("Thread %d executing task!\n",omp_get_thread_num())

    miniomp_task_t* task = TQdequeue(&miniomp_taskqueue);
    pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue);

    exec_task(task);
  }
  else
  {
    pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue);
  }
}

void exec_task(miniomp_task_t* task)
{
    miniomp_task_references* ref = (miniomp_task_references* )pthread_getspecific(miniomp_task_references_key);
    miniomp_task_references* new_ref = task->ref;

    // We are executing a new task
    __sync_fetch_and_add(&miniomp_taskqueue.in_execution,1);

    pthread_setspecific(miniomp_task_references_key,(void*)new_ref);
    task->fn(task->data);
    pthread_setspecific(miniomp_task_references_key,(void*)ref);

    // We are done with the execution
    miniomp_task_references* parent = (miniomp_task_references*)task->ref->parent;
    __sync_fetch_and_sub(&parent->running,1UL);
    __sync_fetch_and_sub(&miniomp_taskqueue.in_execution,1);
    if(new_ref->parent_taskgroup_running)
        __sync_fetch_and_sub(task->ref->parent_taskgroup_running,1UL);
    
    // Save the task to free it later
    miniomp_linked_list_node_t * node = (miniomp_linked_list_node_t*)malloc(sizeof(miniomp_linked_list_node_t));
    node->data = (void*)task;
    node->next = NULL;
    node->deallocator = NULL;
    miniomp_linked_list_add(&miniomp_task_allocations,node);
}

void exec_task_now(void (*fn)(void *), void (*data))
{
    // We have to create the reference in this case
    miniomp_task_references* ref = (miniomp_task_references*)pthread_getspecific(miniomp_task_references_key);
    miniomp_task_references* new_ref = (miniomp_task_references*)malloc(sizeof(miniomp_task_references));
    new_ref->parent = ref;
    new_ref->running = 0;
    new_ref->taskgroup_running = 0;
    if(ref->parent_taskgroup_running)
    {
        new_ref->parent_taskgroup_running = ref->parent_taskgroup_running;
    }
    else 
    {
        new_ref->parent_taskgroup_running = NULL;
    }
    store_ref_in_list(new_ref,&miniomp_task_allocations); //to free later

    // We are executing a new task
    __sync_fetch_and_add(&miniomp_taskqueue.in_execution,1);

    __sync_fetch_and_add(&ref->running,1UL);
    if(new_ref->parent_taskgroup_running)
        __sync_fetch_and_add(new_ref->parent_taskgroup_running,1UL);

    pthread_setspecific(miniomp_task_references_key,(void*)new_ref);
    fn(data);
    pthread_setspecific(miniomp_task_references_key,(void*)ref);

    // We are done with the task
    __sync_fetch_and_sub(&ref->running,1UL);
    __sync_fetch_and_sub(&miniomp_taskqueue.in_execution,1);
    if(new_ref->parent_taskgroup_running)
        __sync_fetch_and_sub(new_ref->parent_taskgroup_running,1UL);
}

void store_ref_in_list(miniomp_task_references* ref, miniomp_linked_list_t* list)
{
    miniomp_linked_list_node_t * node = (miniomp_linked_list_node_t*)malloc(sizeof(miniomp_linked_list_node_t));
    node->data = (void*)ref;
    node->next = NULL;
    node->deallocator = NULL;

    miniomp_linked_list_add(&miniomp_task_allocations,node);
}