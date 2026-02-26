CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

.PHONY: all clean run test

all: compiler

compiler: compiler.cpp
	$(CXX) $(CXXFLAGS) -o compiler compiler.cpp

# Roda sem argumentos (usa programa embutido)
run: compiler
	./compiler

# Roda com o arquivo de exemplo
test: compiler
	./compiler exemplo.wn

clean:
	rm -f compiler teste.mem exemplo.mem saida.mem
