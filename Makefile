CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

.PHONY: all clean run

all: compiler

compiler: compiler.cpp
	$(CXX) $(CXXFLAGS) -o compiler compiler.cpp

# Compila e roda o exemplo do PDF
run: compiler
	./compiler exemplo.wn saida.mem

clean:
	rm -f compiler saida.mem
