CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

chord: node.cpp main.cpp node.h
	$(CXX) $(CXXFLAGS) -o chord node.cpp main.cpp

clean:
	rm -f chord actual_output.txt

test: chord
	./chord > actual_output.txt
	diff actual_output.txt expected_output.txt && echo "ALL TESTS PASS" || echo "DIFF FOUND"
