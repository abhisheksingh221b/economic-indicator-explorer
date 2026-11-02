# make       -> compile the program
# make run   -> compile and start it
# make test  -> compile and run the self-test
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

explorer: src/main.cpp src/*.h
	$(CXX) $(CXXFLAGS) -o explorer src/main.cpp

run: explorer
	./explorer data/countries_2023.csv

test: tests/test_all.cpp src/*.h
	$(CXX) $(CXXFLAGS) -Isrc -o test_all tests/test_all.cpp
	./test_all

clean:
	rm -f explorer test_all
