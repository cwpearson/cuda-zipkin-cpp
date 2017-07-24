#! /bin/bash

LD_PRELOAD="$LD_PRELOAD:/usr/local/cuda/extras/CUPTI/lib64/libcupti.so"
LD_PRELOAD="$LD_PRELOAD:$PWD/prof.so" $@
