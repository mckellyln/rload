
CXX = g++
CXXFLAGS = -Wall -g -O2 -std=c++11

all:	logstats net

rload:	rload.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

logstats:	logstats.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

net:	net.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

clean:
		rm -f rload rload.o logstats logstats.o net net.o
