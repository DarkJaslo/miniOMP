CC = gcc

CFLAGS   = -Wall -O -g -std=c99 -march=native -I. -D_GNU_SOURCE
CFLAGS_SO = -fPIC --shared

MINIOMP = ../
TARGETS  = libminiomp.so
all: $(TARGETS) 

libminiomp.so: libminiomp.h libminiomp.c env.c intrinsic.c parallel.c synchronization.c single.c task.c taskloop.c tasksync.c taskreduction.c linked_list.h linked_list.c
	$(CC) $(CFLAGS_SO) $(CFLAGS) $+ -o $(MINIOMP)/lib/$@ -lpthread 

clean:
	rm -rf $(MINIOMP)/lib/$(TARGETS)
