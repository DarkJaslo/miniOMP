if [ $# -ne 1 ]; then
    echo "Usage: exec <path-to-executable>"
    exit
fi

LD_LIBRARY_PATH=../lib $1