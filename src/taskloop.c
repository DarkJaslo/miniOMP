#include "libminiomp.h"

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

void GOMP_taskgroup_reduction_register (uintptr_t *data);

// Called when encountering a taskloop directive
void
GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
               long arg_size, long arg_align, unsigned flags,
               unsigned long num_tasks, int priority,
               long start, long end, long step)
{
    printf("TBI: a taskloop has been encountered, with ");
    if (flags & GOMP_TASK_FLAG_GRAINSIZE) {
        printf("grainsize=%ld, ", num_tasks);
    } else {
        if (num_tasks == 0) num_tasks = omp_get_num_threads();
        printf("num_tasks=%ld, ", num_tasks);
    }
    printf("I am executing it immediately\n");
	
    // In case the taskloop defines a scope for task reductions, we need to allocate
    // the register and allocate the appropriate per-thread copies
    if (flags & GOMP_TASK_FLAG_REDUCTION)
	{
	  struct data_head { long t1, t2; uintptr_t *ptr; };
	  uintptr_t *ptr = ((struct data_head *) data)->ptr;
	  GOMP_taskgroup_reduction_register(ptr);
	}

    // This part of the code appropriately copies data to be passed to task function,
    // either using a compiler cpyfn function or just memcopy otherwise; no need to
    // fully understand it for the purposes of this assignment
    char *arg;
    if (__builtin_expect (cpyfn != NULL, 0)) {
	  char * buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          arg        = (char *) (((uintptr_t) buf + arg_align - 1)
                                & ~(uintptr_t) (arg_align - 1));
          cpyfn (arg, data);
    } else {
          arg        =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          memcpy (arg, data, arg_size);
    }

    // Function invocation should be replaced with the appropriate task(s) instatiation(s)
    ((long *)arg)[0] = start;
    ((long *)arg)[1] = end;
    fn (arg);
}
