// =============================================================
// compiler.cpp
// Compilador Hipotetico -> Maquina NEANDERWIN
// Disciplina: Compiladores - Ciencia da Computacao
//
// Fases implementadas:
//   1. Analise Lexica
//   2. Analise Sintatica (descida recursiva)
//   3. Analise Semantica (tabela de simbolos)
//   4. Geracao de Codigo NEANDERWIN
// =============================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <stdexcept>
#include <cctype>
#include <iomanip>
#include <algorithm>
#include <cstring>     // memset, memcpy
#include <cstdint>     // int32_t
#ifndef _WIN32
#include <sys/stat.h>  // chmod
#endif

// =============================================================
// SECAO 1: ANALISE LEXICA
// =============================================================

enum class TipoToken {
    // Palavras reservadas
    BEGIN, END,
    IF, THEN, ENDIF,
    WHILE, DO, ENDWHILE,
    // Literais e identificadores
    IDENTIFICADOR,
    NUMERO,
    // Operadores
    ATRIBUICAO,  // =
    MAIS,        // +
    MENOS,       // -
    // Fim de arquivo
    FIM
};

std::string nomeTipo(TipoToken t) {
    switch (t) {
        case TipoToken::BEGIN:         return "BEGIN";
        case TipoToken::END:           return "END";
        case TipoToken::IF:            return "IF";
        case TipoToken::THEN:          return "THEN";
        case TipoToken::ENDIF:         return "ENDIF";
        case TipoToken::WHILE:         return "WHILE";
        case TipoToken::DO:            return "DO";
        case TipoToken::ENDWHILE:      return "ENDWHILE";
        case TipoToken::IDENTIFICADOR: return "IDENTIFICADOR";
        case TipoToken::NUMERO:        return "NUMERO";
        case TipoToken::ATRIBUICAO:    return "ATRIBUICAO (=)";
        case TipoToken::MAIS:          return "MAIS (+)";
        case TipoToken::MENOS:         return "MENOS (-)";
        case TipoToken::FIM:           return "FIM";
    }
    return "?";
}

struct Token {
    TipoToken tipo;
    std::string valor;
    int linha;
};

// Analisador Lexico: converte codigo-fonte em sequencia de tokens
class AnalisadorLexico {
public:
    explicit AnalisadorLexico(const std::string& fonte)
        : fonte(fonte), pos(0), linha(1) {}

    std::vector<Token> tokenizar() {
        std::vector<Token> tokens;

        while (pos < fonte.size()) {
            pularEspacos();
            if (pos >= fonte.size()) break;

            char c = fonte[pos];

            if (c == '\n') {
                linha++;
                pos++;
                continue;
            }

            if (std::isalpha(c) || c == '_') {
                tokens.push_back(lerPalavra());
            } else if (std::isdigit(c)) {
                tokens.push_back(lerNumero());
            } else if (c == '=') {
                tokens.push_back({TipoToken::ATRIBUICAO, "=", linha});
                pos++;
            } else if (c == '+') {
                tokens.push_back({TipoToken::MAIS, "+", linha});
                pos++;
            } else if (c == '-') {
                tokens.push_back({TipoToken::MENOS, "-", linha});
                pos++;
            } else {
                throw std::runtime_error(
                    "Erro lexico na linha " + std::to_string(linha) +
                    ": caractere invalido '" + std::string(1, c) + "'");
            }
        }

        tokens.push_back({TipoToken::FIM, "EOF", linha});
        return tokens;
    }

private:
    std::string fonte;
    size_t pos;
    int linha;

    void pularEspacos() {
        while (pos < fonte.size() &&
               (fonte[pos] == ' ' || fonte[pos] == '\t' || fonte[pos] == '\r'))
            pos++;
    }

    Token lerPalavra() {
        size_t inicio = pos;
        while (pos < fonte.size() && (std::isalnum(fonte[pos]) || fonte[pos] == '_'))
            pos++;

        std::string palavra = fonte.substr(inicio, pos - inicio);

        // Verificar palavras reservadas
        if (palavra == "BEGIN")    return {TipoToken::BEGIN,    palavra, linha};
        if (palavra == "END")      return {TipoToken::END,      palavra, linha};
        if (palavra == "IF")       return {TipoToken::IF,       palavra, linha};
        if (palavra == "THEN")     return {TipoToken::THEN,     palavra, linha};
        if (palavra == "ENDIF")    return {TipoToken::ENDIF,    palavra, linha};
        if (palavra == "WHILE")    return {TipoToken::WHILE,    palavra, linha};
        if (palavra == "DO")       return {TipoToken::DO,       palavra, linha};
        if (palavra == "ENDWHILE") return {TipoToken::ENDWHILE, palavra, linha};

        return {TipoToken::IDENTIFICADOR, palavra, linha};
    }

    Token lerNumero() {
        size_t inicio = pos;
        while (pos < fonte.size() && std::isdigit(fonte[pos]))
            pos++;
        return {TipoToken::NUMERO, fonte.substr(inicio, pos - inicio), linha};
    }
};

// =============================================================
// SECAO 2: TABELA DE SIMBOLOS
// =============================================================

// Gerencia variaveis e constantes, associando-as a enderecos de memoria
class TabelaDeSimbolos {
public:
    // Area de variaveis: enderecos 128-199
    // Area de constantes: enderecos 200-255
    TabelaDeSimbolos() : proxVar(128), proxConst(200) {}

    // Retorna endereco da variavel (cria na tabela se ainda nao existir)
    int obterVar(const std::string& nome) {
        if (vars.find(nome) == vars.end())
            vars[nome] = proxVar++;
        return vars[nome];
    }

    // Retorna endereco da constante inteira (cria se ainda nao existir)
    int obterConst(int valor) {
        if (consts.find(valor) == consts.end())
            consts[valor] = proxConst++;
        return consts[valor];
    }

    void imprimir() const {
        const int W = 48;
        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "            TABELA DE SIMBOLOS\n";
        std::cout << std::string(W, '=') << "\n";
        std::cout << std::left
                  << std::setw(16) << "Nome"
                  << std::setw(16) << "Tipo"
                  << "Endereco\n";
        std::cout << std::string(W, '-') << "\n";

        for (auto& [nome, addr] : vars)
            std::cout << std::setw(16) << nome
                      << std::setw(16) << "variavel"
                      << addr << "\n";

        for (auto& [val, addr] : consts) {
            std::string nome = "CONST_" + std::to_string(val);
            std::cout << std::setw(16) << nome
                      << std::setw(16) << "constante"
                      << addr << "  (valor=" << val << ")\n";
        }

        std::cout << std::string(W, '=') << "\n";
    }

    const std::map<std::string, int>& getVars()   const { return vars; }
    const std::map<int, int>&         getConsts()  const { return consts; }

private:
    std::map<std::string, int> vars;
    std::map<int, int>         consts;
    int proxVar, proxConst;
};

// =============================================================
// SECAO 3: GERACAO DE CODIGO
// =============================================================

struct Instrucao {
    std::vector<std::string> rotulos;  // pode haver mais de um rotulo por instrucao
    std::string mnemonico;
    std::string operando;              // endereco numerico ou rotulo simbolico
};

// Gera instrucoes NEANDERWIN com rotulos simbolicos
// Resolve rotulos na segunda passagem ao gerar a imagem de memoria
class GeradorDeCodigo {
public:
    GeradorDeCodigo() : contadorRotulos(0) {}

    // Cria um novo rotulo unico com o prefixo dado
    std::string novoRotulo(const std::string& prefixo) {
        return prefixo + "_" + std::to_string(contadorRotulos++);
    }

    // Define que o proximo emit recebera este rotulo
    void definirRotulo(const std::string& rotulo) {
        pendentes.push_back(rotulo);
    }

    // Emite uma instrucao, aplicando qualquer rotulo pendente
    void emitir(const std::string& mnemonico, const std::string& operando = "") {
        instrucoes.push_back({pendentes, mnemonico, operando});
        pendentes.clear();
    }

    void imprimir() const {
        const int W = 48;
        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "       CODIGO ASSEMBLY NEANDERWIN GERADO\n";
        std::cout << std::string(W, '=') << "\n";

        for (auto& i : instrucoes) {
            for (auto& r : i.rotulos)
                std::cout << r << ":\n";
            std::cout << "    " << std::left << std::setw(8) << i.mnemonico;
            if (!i.operando.empty())
                std::cout << i.operando;
            std::cout << "\n";
        }

        std::cout << std::string(W, '=') << "\n";
    }

    // Gera e exibe a imagem numerica de memoria (para o simulador NEANDERWIN)
    void imprimirImagemDeMemoria(const TabelaDeSimbolos& tabela) const {
        // Opcodes NEANDERWIN (baseado no Neander com extensao SUB)
        // STA=0x10, LDA=0x20, ADD=0x30, SUB=0x40, JMP=0x80, JZ=0xA0, HLT=0xF0
        const std::unordered_map<std::string, int> opcodes = {
            {"STORE", 0x10},
            {"LOAD",  0x20},
            {"ADD",   0x30},
            {"SUB",   0x40},
            {"JMP",   0x80},
            {"JZ",    0xA0},
            {"HALT",  0xF0}
        };

        // 1a passagem: mapear rotulos para enderecos de instrucoes
        std::unordered_map<std::string, int> mapaRotulos;
        int addr = 0;
        for (auto& i : instrucoes) {
            for (auto& r : i.rotulos)
                mapaRotulos[r] = addr;
            // HALT ocupa 1 byte; demais instrucoes ocupam 2 (opcode + operando)
            addr += (i.mnemonico == "HALT") ? 1 : 2;
        }

        // 2a passagem: gerar valores numericos das instrucoes
        std::map<int, int> memoria;
        addr = 0;
        for (auto& i : instrucoes) {
            memoria[addr++] = opcodes.at(i.mnemonico);
            if (i.mnemonico != "HALT")
                memoria[addr++] = resolverOperando(i.operando, mapaRotulos);
        }

        // Area de dados: variaveis inicializadas com 0
        for (auto& [nome, daddr] : tabela.getVars())
            memoria[daddr] = 0;

        // Area de dados: constantes com seus valores reais
        for (auto& [val, daddr] : tabela.getConsts())
            memoria[daddr] = val;

        const int W = 52;
        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "          IMAGEM DE MEMORIA NEANDERWIN\n";
        std::cout << std::string(W, '=') << "\n";
        std::cout << std::left
                  << std::setw(12) << "Endereco"
                  << std::setw(16) << "Valor (decimal)"
                  << "Valor (hex)\n";
        std::cout << std::string(W, '-') << "\n";

        for (auto& [a, v] : memoria) {
            std::cout << std::setw(12) << a
                      << std::setw(16) << v
                      << "0x" << std::right << std::uppercase << std::hex
                      << std::setw(2) << std::setfill('0') << v
                      << std::left << std::dec << std::setfill(' ') << "\n";
        }

        std::cout << std::string(W, '=') << "\n";
    }

    // Exporta a imagem de memoria no formato binario NEANDERWIN (.mem)
    // Formato: 2 bytes header (0x03, 0x00) + 256 x 2 bytes (valor, flag) = 514 bytes
    void exportarMemoria(const std::string& arquivo,
                         const TabelaDeSimbolos& tabela) const {
        std::vector<int> mem = construirMemoria(tabela);
        std::ofstream f(arquivo, std::ios::binary);
        if (!f) {
            std::cerr << "Aviso: nao foi possivel criar '" << arquivo << "'\n";
            return;
        }
        // Header: tipo Neander (0x03) + reservado (0x00)
        unsigned char header[2] = {0x03, 0x00};
        f.write(reinterpret_cast<char*>(header), 2);

        // 256 posicoes: cada uma como 2 bytes (valor, flag=0)
        for (int i = 0; i < 256; i++) {
            unsigned char val = static_cast<unsigned char>(mem[i] & 0xFF);
            unsigned char flag = 0x00;
            f.write(reinterpret_cast<char*>(&val), 1);
            f.write(reinterpret_cast<char*>(&flag), 1);
        }
        f.close();
        std::cout << "Arquivo NEANDERWIN: " << arquivo << " (514 bytes)\n";
    }

    // Gera binarios nativos (Linux ELF + Windows PE) que simulam a execucao NEANDERWIN.
    // SEM chamar nenhum compilador externo: escreve o stub pre-compilado
    // (embutido como bytes) e anexa os dados (vars + memoria) no final.
    void gerarBinarios(const std::string& nomeBase,
                       const TabelaDeSimbolos& tabela) const {
        gerarBinarioLinux(nomeBase, tabela);
        gerarBinarioWindows(nomeBase + ".exe", tabela);
    }

    void gerarBinarioLinux(const std::string& nomeExec,
                           const TabelaDeSimbolos& tabela) const {
        #include "stub/stub_linux_bytes.h"
        escreverBinario(nomeExec, stub_stub_linux, stub_stub_linux_len, tabela, true);
    }

    void gerarBinarioWindows(const std::string& nomeExec,
                             const TabelaDeSimbolos& tabela) const {
        #include "stub/stub_windows_bytes.h"
        escreverBinario(nomeExec, stub_stub_windows_exe, stub_stub_windows_exe_len, tabela, false);
    }

    void escreverBinario(const std::string& nomeExec,
                         const unsigned char* stubData, unsigned int stubLen,
                         const TabelaDeSimbolos& tabela, bool setExecBit) const {
        std::vector<int> mem = construirMemoria(tabela);

        std::ofstream f(nomeExec, std::ios::binary);
        if (!f) {
            std::cerr << "Aviso: nao foi possivel gerar " << nomeExec << "\n";
            return;
        }

        // 1. Escreve o stub (ELF ou PE)
        f.write(reinterpret_cast<const char*>(stubData), stubLen);

        // 2. Serializa e anexa a tabela de variaveis (32 entradas x 24 bytes = 768 bytes)
        const int MAX_VARS = 32;
        struct VarEntry { int32_t addr; int32_t name_len; char name[16]; };

        std::vector<VarEntry> vars(MAX_VARS);
        memset(vars.data(), 0, sizeof(VarEntry) * MAX_VARS);

        int idx = 0;
        for (auto& [nome, addr] : tabela.getVars()) {
            if (idx >= MAX_VARS) break;
            vars[idx].addr = addr;
            vars[idx].name_len = std::min((int)nome.size(), 15);
            memcpy(vars[idx].name, nome.c_str(), vars[idx].name_len);
            idx++;
        }
        for (auto& [val, addr] : tabela.getConsts()) {
            if (idx >= MAX_VARS) break;
            std::string cname = "CONST_" + std::to_string(val);
            vars[idx].addr = addr;
            vars[idx].name_len = std::min((int)cname.size(), 15);
            memcpy(vars[idx].name, cname.c_str(), vars[idx].name_len);
            idx++;
        }
        if (idx < MAX_VARS)
            vars[idx].addr = -1;

        f.write(reinterpret_cast<const char*>(vars.data()), MAX_VARS * sizeof(VarEntry));

        // 3. Serializa e anexa a memoria NEANDERWIN (256 x int32 = 1024 bytes)
        std::vector<int32_t> mem32(256, 0);
        for (int i = 0; i < 256; i++) mem32[i] = static_cast<int32_t>(mem[i]);
        f.write(reinterpret_cast<const char*>(mem32.data()), 256 * sizeof(int32_t));

        f.close();

#ifndef _WIN32
        if (setExecBit)
            chmod(nomeExec.c_str(), 0755);
#else
        (void)setExecBit;
#endif

        std::cout << "Simulador gerado:   " << nomeExec << "\n";
    }

private:
    std::vector<Instrucao>   instrucoes;
    std::vector<std::string> pendentes;
    int contadorRotulos;

    // Constroi o array de 256 bytes com instrucoes + dados
    std::vector<int> construirMemoria(const TabelaDeSimbolos& tabela) const {
        const std::unordered_map<std::string, int> opcodes = {
            {"STORE", 0x10}, {"LOAD", 0x20}, {"ADD", 0x30},
            {"SUB",   0x40}, {"JMP",  0x80}, {"JZ",  0xA0},
            {"HALT",  0xF0}
        };
        std::unordered_map<std::string, int> mapaRotulos;
        int addr = 0;
        for (auto& i : instrucoes) {
            for (auto& r : i.rotulos) mapaRotulos[r] = addr;
            addr += (i.mnemonico == "HALT") ? 1 : 2;
        }
        std::vector<int> mem(256, 0);
        addr = 0;
        for (auto& i : instrucoes) {
            mem[addr++] = opcodes.at(i.mnemonico);
            if (i.mnemonico != "HALT")
                mem[addr++] = resolverOperando(i.operando, mapaRotulos);
        }
        for (auto& [nome, daddr] : tabela.getVars())   if (daddr < 256) mem[daddr] = 0;
        for (auto& [val,  daddr] : tabela.getConsts()) if (daddr < 256) mem[daddr] = val;
        return mem;
    }

    static int resolverOperando(const std::string& op,
                                const std::unordered_map<std::string, int>& mapa) {
        try { return std::stoi(op); } catch (...) {}
        auto it = mapa.find(op);
        if (it != mapa.end()) return it->second;
        throw std::runtime_error("Operando nao resolvido: '" + op + "'");
    }
};

// =============================================================
// SECAO 4a: AUTOMATO FINITO DETERMINISTICO (AFD)
// =============================================================

static void imprimirAFD() {
    const int W = 72;
    std::cout << "\n" << std::string(W, '=') << "\n";
    std::cout << "   AUTOMATO FINITO DETERMINISTICO (AFD) - ANALISADOR LEXICO\n";
    std::cout << std::string(W, '=') << "\n\n";

    std::cout << "Estados:\n";
    std::cout << "  q0 = Estado inicial\n";
    std::cout << "  q1 = Lendo identificador/palavra reservada (estado final)\n";
    std::cout << "  q2 = Lendo numero inteiro (estado final)\n";
    std::cout << "  q3 = Token '=' reconhecido (estado final)\n";
    std::cout << "  q4 = Token '+' reconhecido (estado final)\n";
    std::cout << "  q5 = Token '-' reconhecido (estado final)\n";
    std::cout << "  qE = Estado de erro (caractere invalido)\n\n";

    std::cout << "Estado inicial: q0\n";
    std::cout << "Estados de aceitacao: {q1, q2, q3, q4, q5}\n";
    std::cout << "Alfabeto: {letra, digito, _, =, +, -, espaco, \\n, outro}\n\n";

    std::cout << "Tabela de transicoes:\n";
    std::cout << std::string(W, '-') << "\n";
    std::cout << std::left
              << std::setw(10) << "Estado"
              << std::setw(12) << "letra/_"
              << std::setw(10) << "digito"
              << std::setw(8)  << "="
              << std::setw(8)  << "+"
              << std::setw(8)  << "-"
              << std::setw(10) << "espaco"
              << "outro\n";
    std::cout << std::string(W, '-') << "\n";
    std::cout << std::setw(10) << "q0"
              << std::setw(12) << "q1"
              << std::setw(10) << "q2"
              << std::setw(8)  << "q3*"
              << std::setw(8)  << "q4*"
              << std::setw(8)  << "q5*"
              << std::setw(10) << "q0"
              << "qE\n";
    std::cout << std::setw(10) << "q1"
              << std::setw(12) << "q1"
              << std::setw(10) << "q1"
              << std::setw(8)  << "r*"
              << std::setw(8)  << "r*"
              << std::setw(8)  << "r*"
              << std::setw(10) << "r*"
              << "r*\n";
    std::cout << std::setw(10) << "q2"
              << std::setw(12) << "qE"
              << std::setw(10) << "q2"
              << std::setw(8)  << "r*"
              << std::setw(8)  << "r*"
              << std::setw(8)  << "r*"
              << std::setw(10) << "r*"
              << "qE\n";
    std::cout << std::string(W, '-') << "\n";
    std::cout << "*  = emite token e retorna a q0\n";
    std::cout << "r* = retrai entrada (devolve caractere), emite token e retorna a q0\n\n";

    std::cout << "Palavras reservadas (verificadas apos reconhecer identificador em q1):\n";
    std::cout << "  BEGIN, END, IF, THEN, ENDIF, WHILE, DO, ENDWHILE\n";
    std::cout << std::string(W, '=') << "\n";
}

// =============================================================
// SECAO 4b: ARVORE DE ANALISE SINTATICA
// =============================================================

struct NoArvore {
    std::string nome;
    std::vector<NoArvore*> filhos;

    explicit NoArvore(const std::string& n) : nome(n) {}
    ~NoArvore() { for (auto* f : filhos) delete f; }

    NoArvore* add(const std::string& n) {
        auto* f = new NoArvore(n);
        filhos.push_back(f);
        return f;
    }

    NoArvore* add(NoArvore* f) {
        if (f) filhos.push_back(f);
        return f;
    }
};

static void imprimirArvore(const NoArvore* no,
                           const std::string& prefixo = "",
                           bool ultimo = true,
                           bool raiz = true) {
    if (!no) return;
    if (!raiz)
        std::cout << prefixo << (ultimo ? "└── " : "├── ");
    std::cout << no->nome << "\n";
    std::string novoPrefixo = raiz ? "" : (prefixo + (ultimo ? "    " : "│   "));
    for (size_t i = 0; i < no->filhos.size(); i++) {
        imprimirArvore(no->filhos[i], novoPrefixo,
                       i == no->filhos.size() - 1, false);
    }
}

// =============================================================
// SECAO 4c: ANALISE SINTATICA + SEMANTICA (descida recursiva)
// =============================================================
//
// Gramatica (BNF simplificada):
//   programa    -> BEGIN lista_cmds END
//   lista_cmds  -> cmd lista_cmds | vazio
//   cmd         -> atrib | se | enquanto
//   atrib       -> ID = expr
//   expr        -> termo (('+' | '-') termo)*
//   termo       -> ID | NUMERO
//   se          -> IF ID THEN lista_cmds ENDIF
//   enquanto    -> WHILE ID DO lista_cmds ENDWHILE

class Parser {
public:
    Parser(const std::vector<Token>& tokens,
           TabelaDeSimbolos& tabela,
           GeradorDeCodigo& gerador)
        : tokens(tokens), pos(0), tabela(tabela), gerador(gerador) {}

    // Analisa o programa e retorna a arvore sintatica
    NoArvore* analisar() {
        return parsePrograma();
    }

private:
    std::vector<Token> tokens;
    size_t pos;
    TabelaDeSimbolos& tabela;
    GeradorDeCodigo&  gerador;

    const Token& atual() const { return tokens[pos]; }

    Token consumir(TipoToken esperado) {
        if (atual().tipo != esperado) {
            throw std::runtime_error(
                "Erro sintatico na linha " + std::to_string(atual().linha) +
                ": esperado '" + nomeTipo(esperado) +
                "', encontrado '" + atual().valor + "'");
        }
        return tokens[pos++];
    }

    // programa -> BEGIN lista_cmds END
    NoArvore* parsePrograma() {
        auto* no = new NoArvore("Programa");
        no->add("BEGIN");
        consumir(TipoToken::BEGIN);
        no->add(parseListaCmds());
        consumir(TipoToken::END);
        no->add("END");
        gerador.emitir("HALT");

        if (atual().tipo != TipoToken::FIM)
            throw std::runtime_error(
                "Tokens inesperados apos END na linha " +
                std::to_string(atual().linha));
        return no;
    }

    // lista_cmds -> cmd lista_cmds | vazio
    NoArvore* parseListaCmds() {
        auto* no = new NoArvore("ListaCmds");
        while (atual().tipo == TipoToken::IDENTIFICADOR ||
               atual().tipo == TipoToken::IF ||
               atual().tipo == TipoToken::WHILE) {
            no->add(parseCmd());
        }
        return no;
    }

    // cmd -> atrib | se | enquanto
    NoArvore* parseCmd() {
        if      (atual().tipo == TipoToken::IDENTIFICADOR) return parseAtrib();
        else if (atual().tipo == TipoToken::IF)            return parseSe();
        else if (atual().tipo == TipoToken::WHILE)         return parseEnquanto();
        else
            throw std::runtime_error(
                "Erro sintatico na linha " + std::to_string(atual().linha) +
                ": comando invalido '" + atual().valor + "'");
    }

    // atrib -> ID = expr
    // expr  -> termo (('+' | '-') termo)*
    NoArvore* parseAtrib() {
        auto* no = new NoArvore("Atrib");
        Token dest = consumir(TipoToken::IDENTIFICADOR);
        no->add(dest.valor);
        consumir(TipoToken::ATRIBUICAO);
        no->add("=");

        auto* expr = new NoArvore("Expr");
        expr->add(atual().valor);
        gerador.emitir("LOAD", enderecoTermo());

        while (atual().tipo == TipoToken::MAIS ||
               atual().tipo == TipoToken::MENOS) {
            bool soma = (atual().tipo == TipoToken::MAIS);
            expr->add(soma ? "+" : "-");
            pos++;
            expr->add(atual().valor);
            gerador.emitir(soma ? "ADD" : "SUB", enderecoTermo());
        }

        no->add(expr);
        gerador.emitir("STORE", std::to_string(tabela.obterVar(dest.valor)));
        return no;
    }

    std::string enderecoTermo() {
        if (atual().tipo == TipoToken::IDENTIFICADOR) {
            Token t = consumir(TipoToken::IDENTIFICADOR);
            return std::to_string(tabela.obterVar(t.valor));
        }
        if (atual().tipo == TipoToken::NUMERO) {
            Token t = consumir(TipoToken::NUMERO);
            return std::to_string(tabela.obterConst(std::stoi(t.valor)));
        }
        throw std::runtime_error(
            "Erro sintatico na linha " + std::to_string(atual().linha) +
            ": esperado identificador ou numero, encontrado '" + atual().valor + "'");
    }

    // se -> IF ID THEN lista_cmds ENDIF
    NoArvore* parseSe() {
        auto* no = new NoArvore("If");
        consumir(TipoToken::IF);
        no->add("IF");
        Token cond = consumir(TipoToken::IDENTIFICADOR);
        no->add(cond.valor);
        consumir(TipoToken::THEN);
        no->add("THEN");

        std::string rotEndif = gerador.novoRotulo("ENDIF");
        gerador.emitir("LOAD", std::to_string(tabela.obterVar(cond.valor)));
        gerador.emitir("JZ",   rotEndif);

        no->add(parseListaCmds());

        consumir(TipoToken::ENDIF);
        no->add("ENDIF");
        gerador.definirRotulo(rotEndif);
        return no;
    }

    // enquanto -> WHILE ID DO lista_cmds ENDWHILE
    NoArvore* parseEnquanto() {
        auto* no = new NoArvore("While");
        consumir(TipoToken::WHILE);
        no->add("WHILE");
        Token cond = consumir(TipoToken::IDENTIFICADOR);
        no->add(cond.valor);
        consumir(TipoToken::DO);
        no->add("DO");

        std::string rotIni = gerador.novoRotulo("WHILE");
        std::string rotFim = gerador.novoRotulo("ENDWHILE");

        gerador.definirRotulo(rotIni);
        gerador.emitir("LOAD", std::to_string(tabela.obterVar(cond.valor)));
        gerador.emitir("JZ",   rotFim);

        no->add(parseListaCmds());

        consumir(TipoToken::ENDWHILE);
        no->add("ENDWHILE");
        gerador.emitir("JMP", rotIni);
        gerador.definirRotulo(rotFim);
        return no;
    }
};

// =============================================================
// SECAO 5: PROGRAMA PRINCIPAL
// =============================================================

// Programa de teste embutido (exemplo do PDF)
static const std::string PROGRAMA_TESTE = R"(BEGIN
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
)";

// Extrai o nome base do arquivo sem extensao e sem diretorio
// Ex: "/home/user/meu_prog.wn" -> "meu_prog"
static std::string nomeBase(const std::string& caminho) {
    size_t barra = caminho.find_last_of("/\\");
    std::string nome = (barra == std::string::npos) ? caminho : caminho.substr(barra + 1);
    size_t ponto = nome.rfind('.');
    if (ponto != std::string::npos) nome = nome.substr(0, ponto);
    return nome.empty() ? "saida" : nome;
}

int main(int argc, char* argv[]) {
    std::string fonte;
    std::string nomeArquivo;
    std::string arquivoSaida;

    std::string nomeExec;   // nome do binario simulador gerado

    if (argc >= 2) {
        // Modo normal: arquivo passado como argumento (ou via drag-and-drop)
        nomeArquivo = argv[1];
        std::ifstream arqFonte(nomeArquivo);
        if (!arqFonte) {
            std::cerr << "Erro: nao foi possivel abrir '" << nomeArquivo << "'\n";
            return 1;
        }
        std::stringstream ss;
        ss << arqFonte.rdbuf();
        fonte       = ss.str();
        nomeExec    = nomeBase(nomeArquivo);
        arquivoSaida = nomeExec + ".mem";
    } else {
        // Sem argumentos: compila o programa de teste embutido
        fonte        = PROGRAMA_TESTE;
        nomeArquivo  = "<teste embutido>";
        nomeExec     = "teste";
        arquivoSaida = "teste.mem";
    }


    const int W = 52;
    std::cout << "\n" << std::string(W, '=') << "\n";
    std::cout << "    COMPILADOR HIPOTETICO -> NEANDERWIN\n";
    std::cout << "    Disciplina: Compiladores\n";
    std::cout << std::string(W, '=') << "\n";
    std::cout << "\n=== CODIGO FONTE (" << nomeArquivo << ") ===\n";
    std::cout << fonte << "\n";

    int codigoRetorno = 0;

    try {
        // FASE 1: Analise Lexica
        AnalisadorLexico lexer(fonte);
        std::vector<Token> tokens = lexer.tokenizar();

        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "         TOKENS (ANALISE LEXICA)\n";
        std::cout << std::string(W, '=') << "\n";
        std::cout << std::left
                  << std::setw(8)  << "Linha"
                  << std::setw(22) << "Tipo"
                  << "Valor\n";
        std::cout << std::string(W, '-') << "\n";
        for (auto& t : tokens) {
            if (t.tipo == TipoToken::FIM) break;
            std::cout << std::setw(8)  << t.linha
                      << std::setw(22) << nomeTipo(t.tipo)
                      << t.valor << "\n";
        }
        std::cout << std::string(W, '=') << "\n";

        // AFD do analisador lexico
        imprimirAFD();

        // FASES 2, 3 e 4: Sintatica + Semantica + Geracao de Codigo
        TabelaDeSimbolos tabela;
        GeradorDeCodigo  gerador;
        Parser parser(tokens, tabela, gerador);
        NoArvore* arvore = parser.analisar();

        // Arvore de analise sintatica
        const int W2 = 52;
        std::cout << "\n" << std::string(W2, '=') << "\n";
        std::cout << "       ARVORE DE ANALISE SINTATICA\n";
        std::cout << std::string(W2, '=') << "\n";
        imprimirArvore(arvore);
        std::cout << std::string(W2, '=') << "\n";

        tabela.imprimir();
        gerador.imprimir();
        gerador.imprimirImagemDeMemoria(tabela);
        gerador.exportarMemoria(arquivoSaida, tabela);
        gerador.gerarBinarios(nomeExec, tabela);

        delete arvore;

        std::cout << "\nCompilacao concluida com SUCESSO!\n";
        std::cout << "Execute './" << nomeExec << "' (Linux) ou '" << nomeExec << ".exe' (Windows) para ver a simulacao verbose.\n";
        std::cout << std::string(W, '=') << "\n\n";

    } catch (const std::exception& ex) {
        std::cerr << "\nERRO DE COMPILACAO: " << ex.what() << "\n";
        codigoRetorno = 1;
    }

    return codigoRetorno;
}
