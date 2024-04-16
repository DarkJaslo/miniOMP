#!/bin/bash

#SBATCH --job-name=submit-omp.sh
#SBATCH -D .
#SBATCH --output=submit-omp.sh.o%j
#SBATCH --error=submit-omp.sh.e%j

HOST=$(echo $HOSTNAME | cut -f 1 -d'.')
if [[ ${HOST} == *"6"* ]] || [[ ${HOST} == *"7"* ]] || [[ ${HOST} == *"8"* ]]
then
    echo "Use sbatch to execute this script"
    exit 0
fi

USAGE="\n USAGE: ./submit-omp.sh prog num_threads\n
        prog        -> omp program name\n
        num_threads -> number of threads\n"

if (test $# -lt 2 || test $# -gt 2)
then
	echo -e $USAGE
        exit 0
fi

make $1
export OMP_NUM_THREADS=$2
./$1
