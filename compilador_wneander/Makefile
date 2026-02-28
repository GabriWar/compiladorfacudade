# ============================================================
# Makefile - Compilador Hipotetico para WNEANDER
# ============================================================
# Compativel com:
#   Windows: mingw32-make (MinGW instalado)
#   Linux:   make
#
# Uso:
#   make            -> compila o projeto (gera compilador.exe no Windows)
#   make run        -> compila e executa com o exemplo do PDF
#   make run_arq    -> compila e executa com arquivo exemplo.wn
#   make clean      -> remove arquivos gerados
# ============================================================

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Detectar sistema operacional
ifeq ($(OS),Windows_NT)
    TARGET = compilador.exe
    RM     = del /Q
    RUN    = compilador.exe
else
    TARGET = compilador
    RM     = rm -f
    RUN    = ./compilador
endif

# Regra padrao: compilar
all: $(TARGET)

$(TARGET): compilador.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) compilador.cpp

# Executar com o exemplo embutido (do enunciado PDF)
run: $(TARGET)
	$(RUN)

# Executar com o arquivo exemplo.wn
run_arq: $(TARGET)
	$(RUN) exemplo.wn

# Remover arquivos gerados
clean:
	$(RM) $(TARGET) saida.asm saida.mem

.PHONY: all run run_arq clean
