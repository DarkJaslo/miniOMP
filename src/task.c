#include "libminiomp.h"

miniomp_taskqueue_t miniomp_taskqueue;

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
        fn (arg);
        //free(arg);
    }
    else
    {
        miniomp_task_t* task = (miniomp_task_t*)malloc(sizeof(miniomp_task_t));
        task->data = (void*)arg;
        task->fn = fn;

        pthread_mutex_lock(&miniomp_taskqueue.lock_taskqueue);
        if(TQis_full(&miniomp_taskqueue))
        {
            pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue); 
            __sync_fetch_and_add(&miniomp_taskqueue.in_execution,1); 
            fn(arg);
            __sync_fetch_and_add(&miniomp_taskqueue.in_execution,-1);
        }
        else
        {
            TQenqueue(&miniomp_taskqueue,task);
            pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue); 
        } 
    }  
    //printf("Task by thread %d is done enqueuing!\n", omp_get_thread_num());
}

void exec_task()
{
  pthread_mutex_lock(&miniomp_taskqueue.lock_taskqueue);

  if(!TQis_empty(&miniomp_taskqueue))
  {
    // Grab a task
    //printf("Thread %d executing task!\n",omp_get_thread_num());
    miniomp_task_t* task = TQdequeue(&miniomp_taskqueue);
    //printf("%d 1 moments before disaster\n", omp_get_thread_num());
    __sync_fetch_and_add(&miniomp_taskqueue.in_execution,1);
    //printf("%d 2 moments before disaster\n", omp_get_thread_num());
    pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue);
    //printf("%d 3 moments before disaster\n", omp_get_thread_num());
    task->fn(task->data);
    //free(task->data);
    //printf("%d 4 moments before disaster\n", omp_get_thread_num());
    __sync_fetch_and_add(&miniomp_taskqueue.in_execution,-1);
  }
  else
  {
    pthread_mutex_unlock(&miniomp_taskqueue.lock_taskqueue);
  }
}
