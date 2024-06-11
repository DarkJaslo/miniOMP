if [ $# -ne 2 ]; then
    echo "Usage: exec <path-to-executable> <num_threads>"
    exit
fi

make $1

export LD_LIBRARY_PATH="../lib"
./$1 -n 100000000 -s 100000 -t $2