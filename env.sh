#! /bin/bash

export THRIFT_ROOT=/home/pearson/software/thrift-0.10.0
export RDKAFKA_ROOT=/home/pearson/software/librdkafka-0.11.0
export FOLLY_ROOT=/home/pearson/software/folly-2017.07.24.00

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${FOLLY_ROOT}/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${THRIFT_ROOT}/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${RDKAFKA_ROOT}/lib"

LD_PRELOAD="$LD_PRELOAD:$PWD/prof.so" $@
