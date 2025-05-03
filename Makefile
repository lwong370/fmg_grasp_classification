CXX ?= g++
CC ?= gcc
CXXFLAGS = -Wall -Wconversion -O3 -fPIC -Iliblinear -Iliblinear/blas
LIBS = liblinear/blas/blas.a

# Source files
SRC_CPP = src/predictor.cpp liblinear/linear.cpp liblinear/newton.cpp main/GPIO_Example.c
SRC_C = 
SRC = $(SRC_CPP) $(SRC_C)

# Object files
OBJ = $(SRC_CPP:.cpp=.o) $(SRC_C:.c=.o) 

# Output binary
TARGET = predictor

all: $(TARGET)

# Link the object files to create the target binary
$(TARGET): $(OBJ) liblinear/blas/blas.a
	$(CXX) -o $@ $^ $(LIBS)

# Compile .cpp files
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

liblinear/%.o: liblinear/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# newton.o: newton.cpp newton.h
# 	$(CXX) $(CFLAGS) -c -o newton.o newton.cpp

# linear.o: linear.cpp linear.h
# 	$(CXX) $(CFLAGS) -c -o linear.o linear.cpp

liblinear/blas/blas.a: liblinear/blas/*.c liblinear/blas/*.h
	make -C blas OPTFLAGS='$(CXXFLAGS)' CC='$(CC)';

# Clean up
clean:
	rm -f $(TARGET) $(OBJ)
	rm -f *~ newton.o linear.o 