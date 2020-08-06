
CXX = g++
CXXFLAGS = -Wall -g -O2 -std=c++11

all:	rload

rload:	rload.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

clean:
		rm -f rload rload.o
