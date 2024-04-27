#include "libminiomp.h"

// Library constructor and desctructor
void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

// Declaration of array for storing the basic thread runtime information
miniomp_thread_runtime * miniomp_threads_sync;

// Declaration of array for storing pthread identifiers from pthread_create function
pthread_t *miniomp_threads;

extern sem_t miniomp_parallel_semaphore;

// Idle function for a thread
void * thread_func(void* args)
{
  miniomp_thread_runtime* runtime = (miniomp_thread_runtime*)args;

  pthread_setspecific(miniomp_specifickey,(void*)(long)runtime->id);

  while(!runtime->stop)
  {
    //Wait for condition variable
    pthread_mutex_lock(&runtime->mutex);
  printf("Prewait %d\n", runtime->id);
    sem_post(&miniomp_parallel_semaphore);
    pthread_cond_wait(&runtime->do_work,&runtime->mutex);
  printf("Postwait %d\n", runtime->id);
    //sem_wait(&runtime->semaphore);

    if(runtime->stop)
    {
      pthread_mutex_unlock(&runtime->mutex);
      return (NULL);
    }

    //Do work
    runtime->fn(runtime->data);
    runtime->data = NULL;

    //Notify work done
    runtime->done = 1;
    pthread_mutex_unlock(&runtime->mutex);
  }

  return (NULL);
}

void
init_miniomp(void) {
  printf ("mini-omp is being initialized\n");
  // Parse OMP_NUM_THREADS environment variable to initialize nthreads_var internal control variable
  parse_env();

  // Initialize Pthread thread-specific data, now just used to store the OpenMP thread identifier
  pthread_key_create(&miniomp_specifickey, NULL);
  pthread_setspecific(miniomp_specifickey, (void *) 0); // implicit initial pthread with id=0

  // Initialize pthread and parallel data structures 

  //Create thread pool
  miniomp_threads = (pthread_t*)malloc( miniomp_icv.nthreads_var*sizeof(pthread_t));
  miniomp_threads_sync = (miniomp_thread_runtime*)malloc( miniomp_icv.nthreads_var*sizeof(miniomp_thread_runtime));

  for(int i = 0; i < miniomp_icv.nthreads_var; ++i)
  {
    //Initialize control variables and mutex
    //Set stop to false
    miniomp_thread_runtime* runtime = miniomp_threads_sync+i;

    runtime->stop = 0;
    runtime->done = 0;
    pthread_cond_init(&runtime->do_work,NULL);
    pthread_mutex_init(&runtime->mutex,NULL);
    sem_init(&miniomp_parallel_semaphore,0,0);
    runtime->id = i;

    pthread_create(miniomp_threads+i,NULL,thread_func,(void*)(miniomp_threads_sync+i));
  }

  // Initialize OpenMP default locks and default barrier
  // Initialize OpenMP workdescriptors for single 
  // Initialize OpenMP task queue for task and taskloop
}

void
fini_miniomp(void) {
  // delete Pthread thread-specific data
  pthread_key_delete(miniomp_specifickey);

  for(int i = 0; i < miniomp_icv.nthreads_var; ++i)
  {
    miniomp_threads_sync[i].stop = true;
    pthread_cond_signal(&miniomp_threads_sync[i].do_work); //In case it was waiting
  }

  for(int t = 0; t < miniomp_icv.nthreads_var; ++t)
  {
    pthread_join(miniomp_threads[t],NULL);
  }

  //Destroy mutexes and condition variables before freeing
  for(int t = 0; t < miniomp_icv.nthreads_var; ++t)
  {
    miniomp_thread_runtime* runtime = miniomp_threads_sync;
    pthread_mutex_destroy(&runtime->mutex);
    pthread_cond_destroy(&runtime->do_work);
    sem_destroy(&miniomp_parallel_semaphore);
  }

  // Free thread pool
  free(miniomp_threads);
  free(miniomp_threads_sync);

  // free other data structures allocated during library initialization
  printf ("mini-omp is finalized\n");
}
