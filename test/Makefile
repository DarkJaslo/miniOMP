.SUFFIXES: -omp -gomp -asm
CC = gcc

MINIOMP = ../

OPENMP	 = -fopenmp
CFLAGS   = -Wall -g -O3 -std=gnu99 -march=native -I.
CLIBS    = -L$(MINIOMP)/lib -lminiomp 

.c-omp: 
	$(CC) $(CFLAGS) $(OPENMP) $< -o $@ $(CLIBS) -lm

.c-gomp: 
	$(CC) $(CFLAGS) $(OPENMP) $< -o $@ -lm

.c-asm: 
	$(CC) $(CFLAGS) $(OPENMP) -c -S $< -o $@ $(CLIBS) 

clean:
	rm -rf *-omp *-gomp *-asm 

ultraclean:
	rm -rf *-omp *-gomp *-asm *.prv *.pcf *.row TRACE.mpits set-0 TRACE.sym submit*.o* submit*.e* 
