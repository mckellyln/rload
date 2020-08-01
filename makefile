
CXX = g++
CXXFLAGS = -Wall -O2

all:	rload

rload:	rload.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

clean:
		rm -f rload rload.o
