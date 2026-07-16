#!/bin/sh

# 执行source ./lib.sh 才能生效

echo_usage()
{
    echo "Usage:"
    echo "source ./lib.sh Your Path"
    exit 1
}

# if [ ! -n "$1" ];then
#     echo_usage
#     exit 1
# fi

LD_PRELOAD_PATH=
EXPORT_LIB_PATH=
EXTRA_LIB_DIR=$(pwd)/lib
# EXTRA_LIB_DIR=$(pwd)/3rd/libs
# LOCAL_LIB_PATH=$(pwd)/output/$1/lib
LOCAL_LIB_PATH=

if [ -d $EXTRA_LIB_DIR ]; then
    for file_name in `find $EXTRA_LIB_DIR -type d`
    do
        EXPORT_LIB_PATH="$EXPORT_LIB_PATH$file_name:"
    done
fi

if [ -d $LOCAL_LIB_PATH ]; then
    EXPORT_LIB_PATH="$EXPORT_LIB_PATH$LOCAL_LIB_PATH:"
fi

echo "EXPORT_LIB_PATH is $EXPORT_LIB_PATH"

export LD_LIBRARY_PATH=$EXPORT_LIB_PATH

# LD_PRELOAD_PATH=$(pwd)/output/$1/lib/libprintf.so

# export LD_PRELOAD=$LD_PRELOAD_PATH

# echo "LD_PRELOAD_PATH is $LD_PRELOAD_PATH"

echo "source ./lib.sh Success"