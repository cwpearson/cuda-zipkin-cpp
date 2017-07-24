TARGETS = prof.so

OBJECTS = \
callbacks.o \
preload_cudart.o

DEPS=$(patsubst %.o,%.d,$(OBJECTS))

ZIPKIN_ROOT=/home/pearson/software/zipkin-cpp
THRIFT_ROOT=/home/pearson/software/thrift-0.10.0
RDKAFKA_ROOT=/home/pearson/software/librdkafka-0.11.0
FOLLY_ROOT=/home/pearson/software/folly-2017.07.24.00
CXX = g++
CXXFLAGS= -std=c++11 -g -fno-omit-frame-pointer -Wall -Wextra -Wshadow -Wpedantic -fPIC
NVCC=nvcc
NVCCFLAGS= -std=c++11 -g -arch=sm_35 -Xcompiler -Wall,-Wextra,-fPIC,-fno-omit-frame-pointer
INC = -I/usr/local/cuda/include \
      -I/usr/local/cuda/extras/CUPTI/include \
      -isystem${ZIPKIN_ROOT}/include \
      -isystem${THRIFT_ROOT}/include \
      -isystem${RDKAFKA_ROOT}/include \
      -isystem${FOLLY_ROOT}/include

LIB = -L/usr/local/cuda/extras/CUPTI/lib64 -lcupti -L/usr/local/cuda/lib64 -lcuda -lcudart -lcudadevrt -L${ZIPKIN_ROOT}/lib -lzipkin -ldl

all: $(TARGETS)

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGETS)

prof.so: $(OBJECTS)
	$(CXX) -shared $(LIB) $^ -o $@

%.o : %.cpp
	cppcheck $<
	$(CXX) -MMD -MP $(CXXFLAGS) $(INC) $< -c -o $@

%.o : %.cu
	$(NVCC) -std=c++11 -arch=sm_35 -dc  -Xcompiler -fPIC $^ -o test.o
	$(NVCC) -std=c++11 -arch=sm_35 -Xcompiler -fPIC -dlink test.o -lcudadevrt -lcudart -o $@	

-include $(DEPS)

