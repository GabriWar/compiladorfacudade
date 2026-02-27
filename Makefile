CXX      = g++
CC       = gcc
MINGW_CC  = x86_64-w64-mingw32-gcc
MINGW_CXX = x86_64-w64-mingw32-g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
STUBFLAGS     = -nostdlib -static -Os -fno-stack-protector -fno-asynchronous-unwind-tables -fno-builtin -no-pie
STUBFLAGS_WIN = -nostdlib -Os -fno-stack-protector -fno-asynchronous-unwind-tables -fno-builtin

.PHONY: all clean run test stubs

# Por padrao: compila o compilador Linux (os _bytes.h ja estao no repo)
all: compiler compiler.exe

# ---------- Compilador Linux ----------
compiler: compiler.cpp stub/stub_linux_bytes.h stub/stub_windows_bytes.h
	$(CXX) $(CXXFLAGS) -o compiler compiler.cpp

# ---------- Compilador Windows (cross-compile via MinGW, ou rodar no Windows) ----------
compiler.exe: compiler.cpp stub/stub_linux_bytes.h stub/stub_windows_bytes.h
	$(MINGW_CXX) $(CXXFLAGS) -static -o compiler.exe compiler.cpp

# ---------- Stubs (so precisam ser regemdos se o codigo dos stubs mudar) ----------
# Requer: gcc, x86_64-w64-mingw32-gcc, xxd (ferramentas Linux)
stubs: stub/stub_linux_bytes.h stub/stub_windows_bytes.h

stub/stub_linux_bytes.h: stub/stub_linux.c
	$(CC) $(STUBFLAGS) -o stub/stub_linux stub/stub_linux.c
	xxd -i stub/stub_linux > stub/stub_linux_bytes.h

stub/stub_windows_bytes.h: stub/stub_windows.c
	$(MINGW_CC) $(STUBFLAGS_WIN) -o stub/stub_windows.exe stub/stub_windows.c -lkernel32
	xxd -i stub/stub_windows.exe > stub/stub_windows_bytes.h

# ---------- Utilidades ----------
run: compiler
	./compiler

test: compiler
	./compiler exemplo.wn

clean:
	rm -f compiler compiler.exe
	rm -f teste teste.mem exemplo exemplo.exe exemplo.mem saida.mem stub_test

# Limpa tudo incluindo os stubs (requer Linux + MinGW para rebuildar)
clean-all: clean
	rm -f stub/stub_linux stub/stub_linux_bytes.h
	rm -f stub/stub_windows.exe stub/stub_windows_bytes.h
