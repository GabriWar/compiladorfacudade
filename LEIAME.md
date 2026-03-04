# Compilador Hipotético → NEANDERWIN

**Disciplina:** Compiladores
**Professor:** Júlio Cézar — Centro Universitário Única

---

## O que este compilador faz

Traduz um programa escrito na linguagem hipotética (`.wn`) para instruções da máquina **NEANDERWIN** e gera um simulador executável com execução verbose passo a passo.

---

## Como compilar e rodar (Windows)

### Pré-requisito
Qualquer compilador C++ com suporte a C++17, por exemplo:
- [MinGW-w64 / MSYS2](https://www.msys2.org/) — rode `pacman -S mingw-w64-x86_64-gcc` após instalar

### Passo 1 — Compilar o compilador

Abra o terminal na pasta do projeto e execute:

```
g++ -std=c++17 -O2 -o compiler.exe compiler.cpp
```

### Passo 2 — Compilar um programa `.wn`

```
compiler.exe exemplo.wn
```

### Passo 3 — Executar o simulador

```
exemplo.exe
```

### Passo 4 — Carregar no NEANDERWIN (opcional)

O arquivo `exemplo.mem` gerado pode ser aberto diretamente no simulador NEANDERWIN.

---

## Como compilar e rodar (Linux)

```
g++ -std=c++17 -O2 -o compiler compiler.cpp
./compiler exemplo.wn
./exemplo
```

---

## Saídas geradas pelo compilador

Para cada compilação, o compilador imprime no terminal:

1. **Código-fonte** lido
2. **Tabela de tokens** da análise léxica (tipo + linha + valor)
3. **Autômato Finito Determinístico (AFD)** do analisador léxico (estados, transições, alfabeto)
4. **Árvore de análise sintática** (representação visual em árvore)
5. **Tabela de símbolos** (variáveis e constantes com seus endereços)
6. **Código assembly NEANDERWIN** com rótulos simbólicos
7. **Imagem de memória** (endereço + valor decimal + valor hex)

E grava em disco:
- `.mem` — imagem de memória binária compatível com o simulador NEANDERWIN (514 bytes)
- Executável simulador (Linux e Windows) — trace verbose da execução

---

## Linguagem suportada

```
BEGIN
  a = 3
  b = 0
  WHILE a DO
    b = b + a
    a = a - 1
  ENDWHILE
  IF b THEN
    c = b
  ENDIF
END
```

- Bloco `BEGIN` / `END`
- Atribuição: `variavel = expressao`
- Aritmética: `+` e `-`
- Condicional: `IF <var> THEN ... ENDIF` (condição: valor != 0, sem ELSE)
- Repetição: `WHILE <var> DO ... ENDWHILE` (condição: valor != 0)

---

## Instruções NEANDERWIN geradas

| Instrução | Opcode | Descrição |
|-----------|--------|-----------|
| LOAD X    | 0x20   | ACC <- MEM[X] |
| STORE X   | 0x10   | MEM[X] <- ACC |
| ADD X     | 0x30   | ACC <- ACC + MEM[X] |
| SUB X     | 0x40   | ACC <- ACC - MEM[X] |
| JMP L     | 0x80   | PC <- L (incondicional) |
| JZ L      | 0xA0   | PC <- L se ACC = 0 |
| HALT      | 0xF0   | Para a execução |

**Layout de memória:** endereços 0-127 = código, 128+ = variáveis, 200+ = constantes
