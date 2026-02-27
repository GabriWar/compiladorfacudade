CXX      = g++
CC       = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
STUBFLAGS = -nostdlib -static -Os -fno-stack-protector -fno-asynchronous-unwind-tables -fno-builtin -no-pie

.PHONY: all clean run test

all: compiler

# O stub Linux e compilado primeiro e os bytes sao embutidos via #include
stub/stub_linux_bytes.h: stub/stub_linux.c
	$(CC) $(STUBFLAGS) -o stub/stub_linux stub/stub_linux.c
	xxd -i stub/stub_linux > stub/stub_linux_bytes.h

compiler: compiler.cpp stub/stub_linux_bytes.h
	$(CXX) $(CXXFLAGS) -o compiler compiler.cpp

# Roda sem argumentos (usa programa embutido)
run: compiler
	./compiler

# Roda com o arquivo de exemplo
test: compiler
	./compiler exemplo.wn

clean:
	rm -f compiler teste teste.mem exemplo exemplo.mem saida.mem stub_test
	rm -f stub/stub_linux stub/stub_linux_bytes.h
