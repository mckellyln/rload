
CXX = g++
CXXFLAGS = -Wall -g -O2 -std=c++11

all:	logstats udpstat

rload:	rload.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

logstats:	logstats.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

udpstat:	udpstat.cpp
		$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)

clean:
		rm -f rload rload.o logstats logstats.o udpstat udpstat.o
