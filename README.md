# cuda-zipkin-cpp

## Setup

Install [zipkin-cpp](https://github.com/flier/zipkin-cpp)

Install the CUDA sdk.

## Using

`env.sh` sets up the `LD_PRELOAD` environment before calling whatever arguments it is provided. For example:

    ./env.sh myprogram

`callbacks.cpp` defines callback functions that are executed by CUPTI. It also defines a function that initializes CUPTI.

`preload_cudart.cpp` defines verions of some CUDA runtime functions that the LD_PRELOAD mechanism will use instead of the actual CUDA runtime functions.