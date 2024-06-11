#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef _OPENMP
#include <omp.h>
#endif

double getusec_() {
        struct timeval time;
        gettimeofday(&time, NULL);
        return ((double)time.tv_sec * (double)1e6 + (double)time.tv_usec);
}

#define START_COUNT_TIME stamp = getusec_();
#define STOP_COUNT_TIME stamp = getusec_() - stamp;\
                        stamp = stamp/1e6;

// process only odd numbers of a specified block
unsigned long eratosthenesBlock(const unsigned long from, const unsigned long to)
{
  unsigned long found = 0;
  unsigned long sqrt_to = sqrt(to);

  // 1. create a list of odd natural numbers 3, 5, 7, ... all of them initially marked as potential primes
  const unsigned long memorySize = (to - from + 1) / 2; // only odd numbers
  char * isPrime = (char *) malloc((memorySize) * sizeof(char));
  for (unsigned long i = 0; i < memorySize; i++) isPrime[i] = 1;

  // 2. Starting from i=3, the first unmarked number on the list ...
  for (unsigned long i = 3; i <= sqrt_to; i+=2) {
    // skip multiples of three: 9, 15, 21, 27, ...
    if (i >= 3*3 && i % 3 == 0)
      continue;
    // skip multiples of five
    if (i >= 5*5 && i % 5 == 0)
      continue;
    // skip multiples of seven
    if (i >= 7*7 && i % 7 == 0)
      continue;
    // skip multiples of eleven
    if (i >= 11*11 && i % 11 == 0)
      continue;
    // skip multiples of thirteen
    if (i >= 13*13 && i % 13 == 0)
      continue;

    // skip numbers before current slice
    unsigned long minJ = ((from+i-1)/i)*i;
    if (minJ < i*i) minJ = i*i;
    // start value must be odd
    if ((minJ & 1) == 0) minJ += i;

    // 3. Mark all multiples of i between minJ and to
    for (unsigned long j = minJ; j <= to; j += 2*i) {
      unsigned long index = j - from;
      isPrime[index/2] = 0;
    }
  }

  // 4. The unmarked numbers are primes, count primes
  for (unsigned long i = 0; i < memorySize; i++) found += isPrime[i];
  // 2 is not odd => include on demand
  if (from <= 2) found++;

  // 5. We are done with the isPrime array, free it
  free(isPrime);
  return found;
}

// process slice-by-slice
unsigned long eratosthenes(unsigned long lastNumber, unsigned long sliceSize) {
  unsigned long found = 0;
  // each slice covers [from ... to) (i.e. incl. from but not to)
  
  //omp_set_num_threads(4);
  #pragma omp parallel shared(found) //reduction(+:found) //inicialitza a 0 found privat   //num_threads(4) //OMP_NUM_THREADS=X ./sieve-omp
  { 
  int myid = omp_get_thread_num();
  int howmany = omp_get_num_threads();
  for (unsigned long from = 2+myid*sliceSize; from <= lastNumber; from += (howmany*sliceSize)) {
      unsigned long to = from + sliceSize;
      if (to > lastNumber) to = lastNumber;
      #pragma omp atomic
      found += eratosthenesBlock(from, to);
  }
  }
  return found;
}

void usage(char *prog) {
#ifdef _OPENMP
    printf("%s [-n <range>] [-s <slice_size>] [-t <thread count>] \n", prog);
    printf("      <range> is an integer defining the exploration range: 2 to range (default 100000) \n");
    printf("      <slice_size> to divide the exploration in slices (default equals range) \n");
    printf("      <thread count> is the number of threads to use (default 1) \n");
#else
    printf("%s [-n <range>] [-s <slice_size>] \n", prog);
    printf("      <range> is an integer defining the exploration range: 2 to range (default 100000) \n");
    printf("      <slice_size> to divide the exploration in slices (default rage) \n");
#endif
}

int main(int argc, char ** argv) {
    unsigned long range_max = 100000000;
    unsigned long slice_size = range_max;
#ifdef _OPENMP
    int num_threads = 8;
#endif

    // Process command-line arguments
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-n")==0) {
            range_max = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-s")==0) {
	    slice_size = atoi(argv[++i]);
        }
#ifdef _OPENMP
        else if (strcmp(argv[i], "-t")==0) {
            num_threads = atoi(argv[++i]);
        }
#endif
        else {
            printf("Error: unknown option %s\n", argv[i]);
            usage(argv[0]);
            return 0;
        }
    }

    if (range_max < 2) {
        printf("Error: <range> Must be an integer greater than or equal to 2\n");
        return 0;
    }

    if ((slice_size > range_max) || (slice_size < 2)) {
        printf("Error: <slice_size> Must be an integer greater than or equal to 2 but smaller or equal than range\n");
        return 0;
    }

#ifdef _OPENMP
    if (num_threads < 1) {
        printf("Error: <thread count> Must be a positive value between 1 an %d\n", omp_get_max_threads());
        return 0;
    } else if (num_threads > omp_get_max_threads()) {
        printf("Warning: number of threads limited to maximum available (%d)\n", omp_get_max_threads());
        num_threads = omp_get_max_threads();
    }

    // Make sure we haven't created too many threads.
    unsigned long temp = (range_max - 1) / num_threads;
    if ((1 + temp) < (unsigned long)sqrt((double)range_max)) {
        printf("Error: Too many threads requested!\n"); 
        printf("       The first thread must have a block size >= sqrt(n)\n");
        exit(1);
    }

    omp_set_num_threads(num_threads);
#endif

    printf("Sieving from 2 to %ld in blocks of %ld ... (may take some time)\n", range_max, slice_size);
    // Find and count primes in the range
    double stamp = 0;
    START_COUNT_TIME;
    unsigned long count = eratosthenes(range_max, slice_size);
    STOP_COUNT_TIME;

    // Print the results.
#ifdef _OPENMP
    printf("%s found %ld primes smaller than or equal to %ld in %0.6f seconds using %d threads\n", 
           argv[0], count, range_max, stamp, num_threads);
#else
    printf("%s found %ld primes smaller than or equal to %ld in %0.6f seconds\n", 
           argv[0], count, range_max, stamp);
#endif
    printf ("%0.6f\n", stamp);

    return 0;
}
//5761455