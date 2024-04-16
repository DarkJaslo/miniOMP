if [ $# -ne 1 ]; then
    echo "Usage: time <path-to-executable>"
    exit
fi

time ./exec.sh $1