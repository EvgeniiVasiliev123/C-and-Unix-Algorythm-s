CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

all: traffic

traffic: src/main.cpp src/graph.hpp src/sim.hpp
	$(CXX) $(CXXFLAGS) src/main.cpp -o traffic

run: traffic
	./traffic data results

clean:
	rm -f traffic
