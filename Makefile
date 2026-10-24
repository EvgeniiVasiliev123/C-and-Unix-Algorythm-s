CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

all: sort1 sort2

sort1: src/sort1_counting.cpp
	$(CXX) $(CXXFLAGS) src/sort1_counting.cpp -o sort1

sort2: src/sort2_insertion.cpp
	$(CXX) $(CXXFLAGS) src/sort2_insertion.cpp -o sort2

run: all
	./sort1
	./sort2

clean:
	rm -f sort1 sort2
