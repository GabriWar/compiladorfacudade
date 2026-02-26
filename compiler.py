#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Compilador Hipotético para a Máquina WNEANDER
==============================================
Disciplina : Compiladores
Avaliação  : Trabalho Prático (25 pontos)
Professor  : Júlio Cézar da Silva Costa

Fases implementadas:
  1. Análise Léxica   – tokenização do código-fonte
  2. Análise Sintática – construção da AST
  3. Análise Semântica – tabela de símbolos
  4. Geração de Código – instruções WNEANDER (LOAD, STORE, ADD, SUB, JZ, JMP, HALT)

Uso:
  python3 compiler.py [arquivo_fonte.txt]
  (sem argumentos usa o programa-exemplo do enunciado)
"""

import sys


# ══════════════════════════════════════════════════════════════
# PARTE 1 – ANÁLISE LÉXICA
# ══════════════════════════════════════════════════════════════

# Tipos de tokens
(TK_BEGIN, TK_END,
 TK_IF, TK_THEN, TK_ENDIF,
 TK_WHILE, TK_DO, TK_ENDWHILE,
 TK_IDENT, TK_NUMBER,
 TK_ASSIGN, TK_PLUS, TK_MINUS,
 TK_EOF) = range(14)

TOKEN_NAMES = {
    TK_BEGIN:    'BEGIN',    TK_END:      'END',
    TK_IF:       'IF',       TK_THEN:     'THEN',    TK_ENDIF:    'ENDIF',
    TK_WHILE:    'WHILE',    TK_DO:       'DO',      TK_ENDWHILE: 'ENDWHILE',
    TK_IDENT:    'IDENT',    TK_NUMBER:   'NUMBER',
    TK_ASSIGN:   'ASSIGN',   TK_PLUS:     'PLUS',    TK_MINUS:    'MINUS',
    TK_EOF:      'EOF',
}

KEYWORDS = {
    'BEGIN': TK_BEGIN, 'END': TK_END,
    'IF': TK_IF, 'THEN': TK_THEN, 'ENDIF': TK_ENDIF,
    'WHILE': TK_WHILE, 'DO': TK_DO, 'ENDWHILE': TK_ENDWHILE,
}


class Token:
    def __init__(self, ttype, value, line):
        self.type  = ttype
        self.value = value
        self.line  = line

    def __repr__(self):
        name = TOKEN_NAMES.get(self.type, str(self.type))
        return f'Token({name}, {self.value!r}, L{self.line})'


class LexerError(Exception):
    pass


class Lexer:
    """
    Analisador léxico: converte o código-fonte em uma sequência de tokens.

    Reconhece:
      • Palavras reservadas: BEGIN END IF THEN ENDIF WHILE DO ENDWHILE
      • Identificadores   : [A-Za-z_][A-Za-z0-9_]*
      • Literais inteiros : [0-9]+
      • Operadores        : = + -
    """

    def __init__(self, source: str):
        self.source = source
        self.tokens: list[Token] = []
        self._cur = 0          # índice de consumo
        self._scan()

    def _scan(self):
        src, i, line = self.source, 0, 1
        while i < len(src):
            ch = src[i]

            # Espaços em branco
            if ch in (' ', '\t', '\r'):
                i += 1

            # Quebra de linha
            elif ch == '\n':
                line += 1
                i += 1

            # Identificadores / palavras reservadas
            elif ch.isalpha() or ch == '_':
                j = i
                while i < len(src) and (src[i].isalnum() or src[i] == '_'):
                    i += 1
                lexeme = src[j:i]
                ttype  = KEYWORDS.get(lexeme.upper(), TK_IDENT)
                self.tokens.append(Token(ttype, lexeme, line))

            # Literais numéricos
            elif ch.isdigit():
                j = i
                while i < len(src) and src[i].isdigit():
                    i += 1
                self.tokens.append(Token(TK_NUMBER, int(src[j:i]), line))

            # Operadores
            elif ch == '=':
                self.tokens.append(Token(TK_ASSIGN, '=', line)); i += 1
            elif ch == '+':
                self.tokens.append(Token(TK_PLUS,   '+', line)); i += 1
            elif ch == '-':
                self.tokens.append(Token(TK_MINUS,  '-', line)); i += 1

            else:
                raise LexerError(f'Caractere inválido: {ch!r} na linha {line}')

        self.tokens.append(Token(TK_EOF, None, line))

    # ── interface de consumo ──────────────────────────────────

    def peek(self) -> Token:
        """Retorna o token atual sem consumi-lo."""
        return self.tokens[self._cur]

    def consume(self) -> Token:
        """Consome e retorna o token atual."""
        tok = self.tokens[self._cur]
        if tok.type != TK_EOF:
            self._cur += 1
        return tok

    def expect(self, ttype: int) -> Token:
        """Consome o token atual exigindo que seja do tipo `ttype`."""
        tok = self.consume()
        if tok.type != ttype:
            exp = TOKEN_NAMES.get(ttype, str(ttype))
            got = TOKEN_NAMES.get(tok.type, str(tok.type))
            raise SyntaxError(
                f'Linha {tok.line}: esperado {exp}, encontrado {got} ({tok.value!r})'
            )
        return tok

    def user_tokens(self) -> list[Token]:
        """Retorna os tokens sem o EOF (para exibição)."""
        return [t for t in self.tokens if t.type != TK_EOF]


# ══════════════════════════════════════════════════════════════
# PARTE 2 – NÓS DA ÁRVORE SINTÁTICA ABSTRATA (AST)
# ══════════════════════════════════════════════════════════════

class ProgramNode:
    """Nó raiz: sequência de comandos entre BEGIN … END."""
    def __init__(self, stmts):
        self.stmts = stmts

class AssignNode:
    """Atribuição: IDENT = expr"""
    def __init__(self, var_name: str, expr):
        self.var_name = var_name
        self.expr     = expr

class IfNode:
    """Condicional: IF expr THEN stmt_list ENDIF"""
    def __init__(self, condition, body):
        self.condition = condition
        self.body      = body

class WhileNode:
    """Laço: WHILE expr DO stmt_list ENDWHILE"""
    def __init__(self, condition, body):
        self.condition = condition
        self.body      = body

class BinOpNode:
    """Operação binária: expr ('+' | '-') expr"""
    def __init__(self, left, op: str, right):
        self.left  = left
        self.op    = op
        self.right = right

class VarNode:
    """Referência a variável."""
    def __init__(self, name: str):
        self.name = name

class NumNode:
    """Literal inteiro."""
    def __init__(self, value: int):
        self.value = value


# ══════════════════════════════════════════════════════════════
# PARTE 3 – ANÁLISE SINTÁTICA (PARSER DESCENDENTE RECURSIVO)
# ══════════════════════════════════════════════════════════════

class ParseError(Exception):
    pass


class Parser:
    """
    Analisador sintático descendente recursivo.

    Gramática (BNF simplificada):
        program    ::= BEGIN stmt_list END
        stmt_list  ::= stmt*
        stmt       ::= assign | if_stmt | while_stmt
        assign     ::= IDENT '=' expr
        if_stmt    ::= IF expr THEN stmt_list ENDIF
        while_stmt ::= WHILE expr DO stmt_list ENDWHILE
        expr       ::= term (('+' | '-') term)*      (assoc. esquerda)
        term       ::= IDENT | NUMBER
    """

    _STOP_TOKENS = {TK_END, TK_ENDIF, TK_ENDWHILE, TK_EOF}

    def __init__(self, lexer: Lexer):
        self.lex = lexer

    def parse(self) -> ProgramNode:
        self.lex.expect(TK_BEGIN)
        stmts = self._stmt_list()
        self.lex.expect(TK_END)
        if self.lex.peek().type != TK_EOF:
            tok = self.lex.peek()
            raise ParseError(f'Linha {tok.line}: tokens inesperados após END')
        return ProgramNode(stmts)

    def _stmt_list(self) -> list:
        stmts = []
        while self.lex.peek().type not in self._STOP_TOKENS:
            stmts.append(self._stmt())
        return stmts

    def _stmt(self):
        tok = self.lex.peek()
        if tok.type == TK_IDENT:
            return self._assign()
        elif tok.type == TK_IF:
            return self._if_stmt()
        elif tok.type == TK_WHILE:
            return self._while_stmt()
        else:
            raise ParseError(
                f'Linha {tok.line}: comando inesperado: {tok.value!r}'
            )

    def _assign(self) -> AssignNode:
        name_tok = self.lex.expect(TK_IDENT)
        self.lex.expect(TK_ASSIGN)
        expr = self._expr()
        return AssignNode(name_tok.value, expr)

    def _if_stmt(self) -> IfNode:
        self.lex.expect(TK_IF)
        cond = self._expr()
        self.lex.expect(TK_THEN)
        body = self._stmt_list()
        self.lex.expect(TK_ENDIF)
        return IfNode(cond, body)

    def _while_stmt(self) -> WhileNode:
        self.lex.expect(TK_WHILE)
        cond = self._expr()
        self.lex.expect(TK_DO)
        body = self._stmt_list()
        self.lex.expect(TK_ENDWHILE)
        return WhileNode(cond, body)

    def _expr(self):
        """
        Expressão com associatividade à esquerda:
            a + b - c  →  BinOp(BinOp(a, '+', b), '-', c)
        Isso garante que o lado direito de qualquer BinOpNode
        seja sempre um terminal (VarNode ou NumNode).
        """
        node = self._term()
        while self.lex.peek().type in (TK_PLUS, TK_MINUS):
            op_tok = self.lex.consume()
            right  = self._term()
            node   = BinOpNode(node, op_tok.value, right)
        return node

    def _term(self):
        tok = self.lex.peek()
        if tok.type == TK_IDENT:
            self.lex.consume()
            return VarNode(tok.value)
        elif tok.type == TK_NUMBER:
            self.lex.consume()
            return NumNode(tok.value)
        else:
            raise ParseError(
                f'Linha {tok.line}: valor esperado, encontrado {tok.value!r}'
            )


# ══════════════════════════════════════════════════════════════
# PARTE 4 – ANÁLISE SEMÂNTICA + TABELA DE SÍMBOLOS
# ══════════════════════════════════════════════════════════════

class SemanticError(Exception):
    pass


class SymbolTable:
    """
    Associa nomes de variáveis a endereços de memória relativos.

    Os endereços são relativos (0, 1, 2, …) e serão deslocados
    pelo tamanho do código durante a montagem final.
    """

    def __init__(self):
        self._table: dict[str, int] = {}
        self._next = 0

    def declare(self, name: str) -> int:
        """Declara variável; idempotente (segunda chamada retorna o endereço existente)."""
        if name not in self._table:
            self._table[name] = self._next
            self._next += 1
        return self._table[name]

    def lookup(self, name: str) -> int:
        if name not in self._table:
            raise SemanticError(f'Variável não declarada: {name!r}')
        return self._table[name]

    @property
    def entries(self) -> dict:
        return dict(self._table)

    def __len__(self):
        return len(self._table)


class SemanticAnalyzer:
    """
    Percorre a AST, preenche a tabela de símbolos e verifica o uso
    correto das variáveis.
    """

    def __init__(self):
        self.table = SymbolTable()

    def analyze(self, program: ProgramNode) -> SymbolTable:
        self._stmts(program.stmts)
        return self.table

    def _stmts(self, stmts):
        for s in stmts:
            self._stmt(s)

    def _stmt(self, stmt):
        if isinstance(stmt, AssignNode):
            self._expr(stmt.expr)             # lado direito primeiro
            self.table.declare(stmt.var_name) # variável definida pela atribuição
        elif isinstance(stmt, IfNode):
            self._expr(stmt.condition)
            self._stmts(stmt.body)
        elif isinstance(stmt, WhileNode):
            self._expr(stmt.condition)
            self._stmts(stmt.body)

    def _expr(self, expr):
        if isinstance(expr, VarNode):
            self.table.declare(expr.name)     # auto-declara variáveis referenciadas
        elif isinstance(expr, NumNode):
            pass
        elif isinstance(expr, BinOpNode):
            self._expr(expr.left)
            self._expr(expr.right)


# ══════════════════════════════════════════════════════════════
# PARTE 5 – GERAÇÃO DE CÓDIGO WNEANDER
# ══════════════════════════════════════════════════════════════

class _LabelRef:
    """Referência simbólica a um rótulo de desvio (uso interno)."""
    def __init__(self, name: str):
        self.name = name
    def __repr__(self):
        return f'LabelRef({self.name})'


class CodeGenerator:
    """
    Gerador de código: percorre a AST e emite instruções WNEANDER.

    Conjunto de instruções suportado:
        LOAD  X  – Acumulador ← mem[X]
        STORE X  – mem[X] ← Acumulador
        ADD   X  – Acumulador ← Acumulador + mem[X]
        SUB   X  – Acumulador ← Acumulador − mem[X]
        JZ    L  – Se Acumulador = 0, PC ← L
        JMP   L  – PC ← L  (salto incondicional)
        HALT     – Encerra a execução

    Layout de memória gerado:
        [0 … N-1]   →  seção de código  (N instruções)
        [N … N+k-1] →  seção de dados   (variáveis e constantes)
    """

    def __init__(self, symbol_table: SymbolTable):
        self.sym   = symbol_table
        self._code = []           # items: ('I', opcode, operand) | ('L', label, None)
        self._lcnt = 0            # contador de rótulos gerados

    # ── helpers ──────────────────────────────────────────────

    def _lbl(self) -> str:
        """Gera um rótulo interno único."""
        self._lcnt += 1
        return f'__L{self._lcnt}'

    def _emit(self, opcode: str, operand=None):
        self._code.append(('I', opcode, operand))

    def _mark(self, label: str):
        """Insere marcador de rótulo (não gera instrução, apenas define posição)."""
        self._code.append(('L', label, None))

    def _const(self, value: int) -> str:
        """Retorna o nome simbólico da constante `value`, alocando-a se necessário."""
        sym_name = f'_K{value}'
        self.sym.declare(sym_name)
        return sym_name

    # ── geração de expressões ─────────────────────────────────

    def _load_expr(self, expr):
        """
        Gera código para avaliar `expr` e depositar o resultado no acumulador.

        Graças à associatividade à esquerda do parser, o nó direito de
        qualquer BinOpNode é sempre um terminal (VarNode ou NumNode),
        permitindo que toda cadeia de +/− seja avaliada diretamente com
        o acumulador sem precisar de variáveis temporárias.
        """
        if isinstance(expr, NumNode):
            self._emit('LOAD', self._const(expr.value))

        elif isinstance(expr, VarNode):
            self._emit('LOAD', expr.name)

        elif isinstance(expr, BinOpNode):
            # Avalia o lado esquerdo → acumulador
            self._load_expr(expr.left)
            # Lado direito é sempre um terminal (garantia do parser)
            right = expr.right
            rref  = self._const(right.value) if isinstance(right, NumNode) else right.name
            if expr.op == '+':
                self._emit('ADD', rref)
            else:
                self._emit('SUB', rref)

    # ── geração de comandos ───────────────────────────────────

    def _gen_stmt(self, stmt):
        if isinstance(stmt, AssignNode):
            # ── Atribuição ──────────────────────────────
            # Avalia expressão → acumulador
            # STORE variável
            self._load_expr(stmt.expr)
            self._emit('STORE', stmt.var_name)

        elif isinstance(stmt, IfNode):
            # ── Condicional IF ───────────────────────────
            #   [avalia condição]
            #   JZ  fim_if        ; falso → pula o corpo
            #   [corpo]
            # fim_if:
            lbl_end = self._lbl()
            self._load_expr(stmt.condition)
            self._emit('JZ', _LabelRef(lbl_end))
            for s in stmt.body:
                self._gen_stmt(s)
            self._mark(lbl_end)

        elif isinstance(stmt, WhileNode):
            # ── Laço WHILE ──────────────────────────────
            # inicio:
            #   [avalia condição]
            #   JZ  fim           ; falso → sai do laço
            #   [corpo]
            #   JMP inicio
            # fim:
            lbl_start = self._lbl()
            lbl_end   = self._lbl()
            self._mark(lbl_start)
            self._load_expr(stmt.condition)
            self._emit('JZ',  _LabelRef(lbl_end))
            for s in stmt.body:
                self._gen_stmt(s)
            self._emit('JMP', _LabelRef(lbl_start))
            self._mark(lbl_end)

    def generate(self, program: ProgramNode):
        """Percorre toda a AST e emite instruções."""
        for stmt in program.stmts:
            self._gen_stmt(stmt)
        self._emit('HALT')

    # ── montagem final (two-pass) ─────────────────────────────

    def assemble(self):
        """
        Resolve rótulos e endereços simbólicos, produzindo a lista final
        de instruções com operandos numéricos absolutos.

        Retorna:
            instructions : list[(opcode, operand | None)]
            final_sym    : dict[name -> abs_address]
        """
        # Passo 1: contar instruções reais (exclui marcadores de rótulo)
        code_size = sum(1 for item in self._code if item[0] == 'I')
        data_base = code_size

        # Passo 2: calcular endereços absolutos dos símbolos
        final_sym = {
            name: data_base + rel
            for name, rel in self.sym.entries.items()
        }

        # Passo 3: calcular endereços absolutos dos rótulos
        label_addr: dict[str, int] = {}
        pos = 0
        for kind, name, _ in self._code:
            if kind == 'L':
                label_addr[name] = pos
            else:
                pos += 1

        # Passo 4: gerar lista final resolvida
        result = []
        for kind, opcode, operand in self._code:
            if kind == 'L':
                continue
            if operand is None:
                result.append((opcode, None))
            elif isinstance(operand, _LabelRef):
                result.append((opcode, label_addr[operand.name]))
            else:
                # operand é o nome simbólico da variável/constante
                result.append((opcode, final_sym[operand]))

        # Atualiza tabela de símbolos com endereços absolutos (para relatório)
        self.sym._table = final_sym
        self.sym._next  = data_base + self.sym._next

        return result, final_sym


# ══════════════════════════════════════════════════════════════
# PARTE 6 – RELATÓRIO DE SAÍDA
# ══════════════════════════════════════════════════════════════

def _sep(char='═', n=62):
    return char * n


def print_report(source: str,
                 tokens: list,
                 instructions: list,
                 final_sym: dict):

    print(_sep())
    print('  COMPILADOR HIPOTÉTICO — MÁQUINA WNEANDER')
    print(_sep())

    # ── Código-fonte ──────────────────────────────────────────
    print()
    print('[ 1. CÓDIGO-FONTE ]')
    print(_sep('─'))
    for i, line in enumerate(source.strip().splitlines(), 1):
        print(f'  {i:3}│  {line}')

    # ── Tokens ───────────────────────────────────────────────
    print()
    print('[ 2. ANÁLISE LÉXICA — TOKENS IDENTIFICADOS ]')
    print(_sep('─'))
    print(f'  {"#":>4}  {"Tipo":<12}  {"Valor":<16}  {"Linha":>5}')
    print(f'  {"─"*4}  {"─"*12}  {"─"*16}  {"─"*5}')
    for i, tok in enumerate(tokens, 1):
        tname = TOKEN_NAMES.get(tok.type, str(tok.type))
        print(f'  {i:>4}  {tname:<12}  {str(tok.value):<16}  {tok.line:>5}')

    # ── Tabela de símbolos ────────────────────────────────────
    print()
    print('[ 3. ANÁLISE SEMÂNTICA — TABELA DE SÍMBOLOS ]')
    print(_sep('─'))
    print(f'  {"Símbolo":<18}  {"Tipo":<12}  {"End. (abs.)":>12}  {"Val. Inicial":>12}')
    print(f'  {"─"*18}  {"─"*12}  {"─"*12}  {"─"*12}')
    for name, addr in sorted(final_sym.items(), key=lambda x: x[1]):
        if name.startswith('_K'):
            val   = int(name[2:])
            tipo  = 'constante'
            vinit = str(val)
            exib  = f'_K{val}'
        else:
            tipo  = 'variável'
            vinit = '0'
            exib  = name
        print(f'  {exib:<18}  {tipo:<12}  {addr:>12}  {vinit:>12}')

    # ── Código WNEANDER gerado ────────────────────────────────
    print()
    print('[ 4. GERAÇÃO DE CÓDIGO — INSTRUÇÕES WNEANDER ]')
    print(_sep('─'))
    print(f'  {"End.":>5}  {"Instrução":<8}  {"Operando":>8}  Comentário')
    print(f'  {"─"*5}  {"─"*8}  {"─"*8}  {"─"*20}')
    # Mapa reverso: endereço → nome (para comentários)
    addr_to_name = {v: k for k, v in final_sym.items()}
    for addr, (opcode, operand) in enumerate(instructions):
        op_str   = str(operand) if operand is not None else ''
        comment  = ''
        if operand is not None and operand in addr_to_name:
            sym = addr_to_name[operand]
            if sym.startswith('_K'):
                comment = f'; constante {sym[2:]}'
            else:
                comment = f'; var "{sym}"'
        print(f'  {addr:>5}  {opcode:<8}  {op_str:>8}  {comment}')

    # ── Mapa de memória inicial ───────────────────────────────
    print()
    print('[ 5. MAPA DE MEMÓRIA INICIAL ]')
    print(_sep('─'))
    print('  Seção de Código (endereços 0 …):')
    for i, (opcode, operand) in enumerate(instructions):
        op_str = str(operand) if operand is not None else ''
        print(f'    mem[{i:>3}] = {opcode} {op_str}')

    code_size = len(instructions)
    print()
    print(f'  Seção de Dados (endereços {code_size} …):')
    print(f'  (carregar estes valores no simulador antes de executar)')
    print(f'  {"End.":>8}  {"Valor":>8}  Descrição')
    print(f'  {"─"*8}  {"─"*8}  {"─"*30}')
    for name, addr in sorted(final_sym.items(), key=lambda x: x[1]):
        if name.startswith('_K'):
            val  = int(name[2:])
            desc = f'constante {val}'
        else:
            val  = 0
            desc = f'variável "{name}" (inicializada em 0)'
        print(f'  {addr:>8}  {val:>8}  {desc}')

    print()
    print(_sep())
    print('  Compilação concluída com sucesso.')
    print(_sep())


# ══════════════════════════════════════════════════════════════
# PONTO DE ENTRADA
# ══════════════════════════════════════════════════════════════

def compile_source(source: str):
    """Executa todas as fases do compilador e imprime o relatório."""

    print('\n>>> Iniciando compilação...\n')

    # ── Fase 1: Análise Léxica ────────────────────────────────
    try:
        lexer  = Lexer(source)
        tokens = lexer.user_tokens()
        print(f'  [LÉXICO]    {len(tokens)} token(s) identificado(s). ✓')
    except LexerError as e:
        print(f'\n  ERRO LÉXICO: {e}')
        sys.exit(1)

    # ── Fase 2: Análise Sintática ─────────────────────────────
    try:
        parser = Parser(lexer)
        ast    = parser.parse()
        print('  [SINTÁTICO] Estrutura do programa válida. ✓')
    except (SyntaxError, ParseError) as e:
        print(f'\n  ERRO SINTÁTICO: {e}')
        sys.exit(1)

    # ── Fase 3: Análise Semântica ─────────────────────────────
    try:
        analyzer   = SemanticAnalyzer()
        sym_table  = analyzer.analyze(ast)
        n_vars     = sum(1 for k in sym_table.entries if not k.startswith('_K'))
        print(f'  [SEMÂNTICO] {n_vars} variável(is) declarada(s). ✓')
    except SemanticError as e:
        print(f'\n  ERRO SEMÂNTICO: {e}')
        sys.exit(1)

    # ── Fase 4: Geração de Código ─────────────────────────────
    try:
        codegen      = CodeGenerator(sym_table)
        codegen.generate(ast)
        instructions, final_sym = codegen.assemble()
        print(f'  [CÓDIGO]    {len(instructions)} instrução(ões) gerada(s). ✓\n')
    except Exception as e:
        print(f'\n  ERRO NA GERAÇÃO DE CÓDIGO: {e}')
        raise

    # ── Relatório ─────────────────────────────────────────────
    print_report(source, tokens, instructions, final_sym)
    return instructions, final_sym


# ── Programa-exemplo do enunciado (PDF) ──────────────────────
EXEMPLO_PDF = """\
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
END"""


if __name__ == '__main__':
    if len(sys.argv) > 1:
        try:
            with open(sys.argv[1], 'r', encoding='utf-8') as f:
                src = f.read()
        except FileNotFoundError:
            print(f'Erro: arquivo {sys.argv[1]!r} não encontrado.')
            sys.exit(1)
    else:
        src = EXEMPLO_PDF

    compile_source(src)
