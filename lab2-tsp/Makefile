CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic

.PHONY: all test run clean
all: tsp test_tsp

tsp: src/main.cpp src/tsp.hpp
	$(CXX) $(CXXFLAGS) src/main.cpp -o tsp

test_tsp: src/test.cpp src/tsp.hpp
	$(CXX) $(CXXFLAGS) src/test.cpp -o test_tsp

test: test_tsp
	./test_tsp

run: tsp
	./tsp data/example.txt

clean:
	rm -f tsp test_tsp
