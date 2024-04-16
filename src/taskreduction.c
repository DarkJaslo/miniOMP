#include "libminiomp.h"
#include <stdlib.h>

/* The format of data is:
   data[0]	cnt              (number of variables in reduction)
   data[1]	size             (size for variables in reduction)
   data[2]	alignment        (on input, alignment for data to be allocated)
                                 (on output, pointer to allocated array for per-thread copies)
   data[3]	used internally
   data[4]	next pointer     (in a list of chained objects of this type)
   data[5]	used internally
   data[6]	used internally
   cnt times
       ent[0]	address          (address for variable in reduction)
       ent[1]	offsert          (offset of the per-thread array for variable in reduction)
       ent[2]	used internally
*/

uintptr_t * miniomp_tg_reduction = 0; // pointer to list of taskgroup reductions

// This funtion registers each variable in the list of reduction variables, allocating enough space 
// to store per-thread copies for each of them. All allocated in consecutive memory locations. 
// data[0] indicates the number of reduction variables for which per-thread copies are required, for
// each copy requiring data[1] bytes. The list of reduction variables starts in data[7]; for each
// variable, ent[0] indicates it address. On return ent[1] should contain the initial address of its
// per-thread array
// On return, data[2] should point to the memory allocated
void
GOMP_taskgroup_reduction_register (uintptr_t *data)
{
    printf("TBI: Entered in GOMP_taskgroup_reduction_register\n");
    if (data[0] != 1) printf("Warning: naive implementation only supporting one reduction variable in taskgroup scope\n");
    printf("    data=%p\n", (void *)data);
    printf("    cnt=%ld\n", data[0]);
    printf("    size=%ld\n", data[1]);
    printf("    alignment=%p\n", (void *)data[2]);
    printf("    allocator=%ld\n", data[3]);
    printf("    next pointer=%p\n", (void *)data[4]);
    printf("    htab pointer=%p\n", (void *)data[5]);
    printf("    end of array=%p\n", (void *)data[6]);
    for (int cnt=0; cnt<data[0]; cnt++) {
        printf("    address=%p\n", (void *)data[7+cnt*3]);
        printf("    offset=%p\n", (void *)data[7+cnt*3+1]);
        printf("    internal=%ld\n", data[7+cnt*3+2]);
    }

#if 0
    // this code needs to be worked out ...
    long sz = data[0] * data[1] * omp_get_num_threads();
    printf("Size of allocated data = %ld\n", sz);
    // Recommend to use aligned_alloc to allocate data
    void *ptr = aligned_alloc(data[2], sz);
    memset (ptr, '\0', sz);
    data[2] = (uintptr_t) ptr;
    data[4] = miniomp_tg_reduction;
    miniomp_tg_reduction = data;
#endif
}

// This funtion unregisters each variable in the list of reduction variables, deallocating 
// the space for them
void
GOMP_taskgroup_reduction_unregister (uintptr_t *data)
{
    printf("TBI: Entered in GOMP_taskgroup_reduction_unregister\n");
    printf("Going to deallocate data at %p\n", (void *)data[2]);

#if 0
    // this code needs to be worked out ...
    free ((void *) data[2]);
    miniomp_tg_reduction = data[4];
#endif
}

/* For i = 0 to cnt-1, remap ptrs[i] which is either address of the
   original list item or address of previously remapped original list
   item to address of the private copy, store that to ptrs[i].
   For i < cntorig, additionally set ptrs[cnt+i] to the address of
   the original list item.  */

void
GOMP_task_reduction_remap (size_t cnt, size_t cntorig, void **ptrs)
{
    printf("TBI: Entered in GOMP_task_reduction_remap with cnt=%ld and cntorig=%ld \n", cnt, cntorig);
    for (int i=0; i<cnt; i++) {
        printf("    ptrs[%d]=%p\n", i, ptrs[i]);
    }
    for (int i=0; i<cntorig; i++) {
        printf("    ptrs[cnt+%d]=%p\n", i, ptrs[cnt+i]);
    }

    // code here needed
}
