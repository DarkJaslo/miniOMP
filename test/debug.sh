if [ $# -ne 1 ]; then
    echo "Usage: exec <path-to-executable>"
    exit
fi

export LD_LIBRARY_PATH="../lib"
export OMP_NUM_THREADS=8
gdb --args $1