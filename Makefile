# make       -> compile the program
# make run   -> compile and start it
# make test  -> compile and run the self-test
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# On Windows the compiler creates "explorer.exe"; on Linux/macOS just "explorer".
# (Windows sets the environment variable OS to Windows_NT.)
ifeq ($(OS),Windows_NT)
EXE = .exe
endif

explorer$(EXE): src/main.cpp src/*.h
	$(CXX) $(CXXFLAGS) -o explorer$(EXE) src/main.cpp

run: explorer$(EXE)
	./explorer$(EXE) data/countries_2023.csv

test: tests/test_all.cpp src/*.h
	$(CXX) $(CXXFLAGS) -Isrc -o test_all$(EXE) tests/test_all.cpp
	./test_all$(EXE)

clean:
	rm -f explorer explorer.exe test_all test_all.exe
