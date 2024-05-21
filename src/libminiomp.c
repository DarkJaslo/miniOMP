#include "libminiomp.h"

// Library constructor and desctructor
void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

// Declaration of array for storing the basic thread runtime information
miniomp_thread_runtime * miniomp_threads_sync;

// Declaration of array for storing pthread identifiers from pthread_create function
pthread_t *miniomp_threads;

// Idle function for a thread
void * thread_func(void* args)
{
  miniomp_thread_runtime* runtime = (miniomp_thread_runtime*)args;

  pthread_setspecific(miniomp_specifickey,(void*)(long)runtime->id);

  while(!runtime->stop)
  {
    //Wait for condition variable
    pthread_mutex_lock(&runtime->mutex);
    // If done is 0, this thread is late to the cond_wait and can continue executing immediately.
    while(runtime->done)
    {
    //printf("Pre-wait %d\n", runtime->id);
      pthread_cond_wait(&runtime->do_work,&runtime->mutex);
    //printf("Post-wait %d\n", runtime->id);
    }
    pthread_mutex_unlock(&runtime->mutex);

    if(runtime->stop)
    {
      return (NULL);
    }

    //Do work
    runtime->fn(runtime->data);

    pthread_mutex_lock(&runtime->mutex);
    runtime->fn = NULL;
    runtime->data = NULL;
    runtime->done = 1;
    pthread_mutex_unlock(&runtime->mutex);

    //miniomp_barrier_wait(&miniomp_parallel_barrier);
    pthread_barrier_wait(&miniomp_parallel_barrier);
  }

  printf("Thread %d is returning\n", runtime->id);
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
  named_criticals.first = NULL;
  named_criticals.last = NULL;

  //Create thread pool
  miniomp_icv.parallel_threads = miniomp_icv.nthreads_var;
  miniomp_threads = (pthread_t*)malloc( miniomp_icv.nthreads_var*sizeof(pthread_t));
  miniomp_threads_sync = (miniomp_thread_runtime*)malloc( miniomp_icv.nthreads_var*sizeof(miniomp_thread_runtime));

  for(int i = 0; i < miniomp_icv.nthreads_var; ++i)
  {
    //Initialize control variables and mutex
    //Set stop to false
    miniomp_thread_runtime* runtime = miniomp_threads_sync+i;

    runtime->stop = 0;
    runtime->done = 1;
    runtime->fn = NULL;
    runtime->data = NULL;
    pthread_cond_init(&runtime->do_work,NULL);
    pthread_mutex_init(&runtime->mutex,NULL);
    runtime->id = i;
    runtime->single_count = 0;

    pthread_create(miniomp_threads+i,NULL,thread_func,(void*)(miniomp_threads_sync+i));
  }

  // Initialize OpenMP default locks and default barrier
  pthread_mutex_init(&miniomp_default_lock, NULL);
  pthread_mutex_init(&miniomp_named_lock, NULL);

  //miniomp_barrier_init(&miniomp_barrier,miniomp_icv.nthreads_var);
  //miniomp_barrier_init(&miniomp_parallel_barrier,miniomp_icv.nthreads_var);
  pthread_barrier_init(&miniomp_barrier, NULL, miniomp_icv.nthreads_var);
  miniomp_barrier_count = miniomp_icv.nthreads_var;
  pthread_barrier_init(&miniomp_parallel_barrier, NULL, miniomp_icv.nthreads_var+1);
  miniomp_barrier_count = miniomp_icv.nthreads_var+1;

  // Initialize OpenMP workdescriptors for single 
  miniomp_single.value = 0;

  // Initialize OpenMP task queue for task and taskloop
}

void
fini_miniomp(void) {
  // delete Pthread thread-specific data
  pthread_key_delete(miniomp_specifickey);

printf("Telling all threads to stop...\n");
  for(int i = 0; i < miniomp_icv.nthreads_var; ++i)
  {
    pthread_mutex_lock(&miniomp_threads_sync[i].mutex);
    miniomp_threads_sync[i].stop = 1;
    miniomp_threads_sync[i].done = 0;
    miniomp_threads_sync[i].fn = NULL;
    pthread_cond_signal(&miniomp_threads_sync[i].do_work); //In case it was waiting
    pthread_mutex_unlock(&miniomp_threads_sync[i].mutex);
  }
printf("Joining all threads...\n");
  for(int t = 0; t < miniomp_icv.nthreads_var; ++t)
  {
    pthread_join(miniomp_threads[t],NULL);
  }
printf("Joined threads. Destroying mutexes and condition variables...\n");

  //Destroy mutexes and condition variables before freeing
  for(int t = 0; t < miniomp_icv.nthreads_var; ++t)
  {
    miniomp_thread_runtime* runtime = miniomp_threads_sync;
    pthread_mutex_destroy(&runtime->mutex);
    pthread_cond_destroy(&runtime->do_work);
  }
printf("Freeing thread pool...\n");
  // Free thread pool
  free(miniomp_threads);
  free(miniomp_threads_sync);

  // free other data structures allocated during library initialization
  pthread_mutex_destroy(&miniomp_default_lock);
  pthread_mutex_destroy(&miniomp_named_lock);
  //miniomp_barrier_destroy(&miniomp_barrier);
  //miniomp_barrier_destroy(&miniomp_parallel_barrier);
  pthread_barrier_destroy(&miniomp_barrier);
  pthread_barrier_destroy(&miniomp_parallel_barrier);

  if(named_criticals.first)
  {
    printf("Freeing named criticals...\n");
    miniomp_linked_list_destroy(named_criticals.first);
  }


printf ("mini-omp is finalized\n");
}
