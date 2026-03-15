CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

chord: node.cpp main.cpp node.h
	$(CXX) $(CXXFLAGS) -o chord node.cpp main.cpp

chord_test: node.cpp tests/test_driver.cpp node.h
	$(CXX) $(CXXFLAGS) -I. -o chord_test node.cpp tests/test_driver.cpp

clean:
	rm -f chord chord_test actual_output.txt

test: chord
	./chord > actual_output.txt
	diff actual_output.txt expected_output.txt && echo "ALL TESTS PASS" || echo "DIFF FOUND"
