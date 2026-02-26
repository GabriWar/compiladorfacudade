// =============================================================
// compiler.cpp
// Compilador Hipotetico -> Maquina WNEANDER
// Disciplina: Compiladores - Ciencia da Computacao
//
// Fases implementadas:
//   1. Analise Lexica
//   2. Analise Sintatica (descida recursiva)
//   3. Analise Semantica (tabela de simbolos)
//   4. Geracao de Codigo WNEANDER
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

// Gera instrucoes WNEANDER com rotulos simbolicos
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
        std::cout << "       CODIGO ASSEMBLY WNEANDER GERADO\n";
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

    // Gera e exibe a imagem numerica de memoria (para o simulador WNEANDER)
    void imprimirImagemDeMemoria(const TabelaDeSimbolos& tabela) const {
        // Opcodes WNEANDER (baseado no Neander com extensao SUB)
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
        std::cout << "          IMAGEM DE MEMORIA WNEANDER\n";
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

    // Exporta a imagem de memoria como arquivo texto (256 linhas: "endereco valor")
    void exportarMemoria(const std::string& arquivo,
                         const TabelaDeSimbolos& tabela) const {
        std::vector<int> mem = construirMemoria(tabela);
        std::ofstream f(arquivo);
        if (!f) {
            std::cerr << "Aviso: nao foi possivel criar '" << arquivo << "'\n";
            return;
        }
        for (int i = 0; i < 256; i++)
            f << i << " " << mem[i] << "\n";
        std::cout << "Imagem de memoria:  " << arquivo << "\n";
    }

    // Gera um binario nativo que simula a execucao WNEANDER com trace verbose.
    // Estrategia: gera um .cpp com a memoria embutida + interpretador, depois
    // chama g++ para compilar. O .cpp intermediario e removido no final.
    void gerarSimulador(const std::string& nomeExec,
                        const TabelaDeSimbolos& tabela) const {
        std::vector<int> mem = construirMemoria(tabela);

        std::string cppFile = nomeExec + ".sim.cpp";
        std::ofstream f(cppFile);
        if (!f) {
            std::cerr << "Aviso: nao foi possivel gerar simulador\n";
            return;
        }

        // --- Cabecalho ---
        f << "#include <iostream>\n"
          << "#include <iomanip>\n"
          << "#include <string>\n"
          << "#include <map>\n\n"
          << "int main() {\n";

        // --- Memoria embutida ---
        f << "    int mem[256] = {};\n";
        for (int i = 0; i < 256; i++)
            if (mem[i] != 0)
                f << "    mem[" << i << "] = " << mem[i] << ";\n";
        f << "\n";

        // --- Tabela de simbolos embutida (para nomes legiveis) ---
        f << "    std::map<int,std::string> vars = {\n";
        for (auto& [nome, addr] : tabela.getVars())
            f << "        {" << addr << ", \"" << nome << "\"},\n";
        for (auto& [val, addr] : tabela.getConsts())
            f << "        {" << addr << ", \"CONST_" << val << "\"},\n";
        f << "    };\n\n";

        // --- Corpo do interpretador WNEANDER (escrito como string literal) ---
        f << R"WNSIM(
    auto nomVar = [&](int a) -> std::string {
        return vars.count(a) ? vars[a] : ("MEM[" + std::to_string(a) + "]");
    };
    auto nomeOp = [](int opc) -> std::string {
        switch(opc) {
            case 0x10: return "STORE";
            case 0x20: return "LOAD";
            case 0x30: return "ADD";
            case 0x40: return "SUB";
            case 0x80: return "JMP";
            case 0xA0: return "JZ";
            case 0xF0: return "HALT";
            default:   return "???";
        }
    };

    int acc = 0, pc = 0, step = 0;

    std::cout << "\n================================================\n";
    std::cout << "   SIMULADOR WNEANDER — execucao verbose\n";
    std::cout << "================================================\n\n";
    std::cout << std::left
              << std::setw(6)  << "PC"
              << std::setw(8)  << "Instr"
              << std::setw(6)  << "Op"
              << std::setw(8)  << "ACC"
              << "Acao\n";
    std::cout << std::string(52, '-') << "\n";

    while (step++ < 100000) {
        int opc = mem[pc];
        bool temOp = (opc != 0xF0);
        int  op    = temOp ? mem[pc + 1] : 0;

        std::cout << std::setw(6) << pc
                  << std::setw(8) << nomeOp(opc);
        if (temOp) std::cout << std::setw(6) << op;
        else       std::cout << std::setw(6) << "";
        std::cout << std::setw(8) << acc;

        if (opc == 0xF0) { std::cout << "HALT\n"; break; }

        switch(opc) {
        case 0x20: { // LOAD
            int v = mem[op];
            std::cout << "ACC = " << nomVar(op) << " = " << v << "\n";
            acc = v; pc += 2; break;
        }
        case 0x10: { // STORE
            mem[op] = acc;
            std::cout << nomVar(op) << " = " << acc << "\n";
            pc += 2; break;
        }
        case 0x30: { // ADD
            int v = mem[op];
            std::cout << "ACC = " << acc << " + " << nomVar(op)
                      << "(" << v << ") = " << (acc + v) << "\n";
            acc += v; pc += 2; break;
        }
        case 0x40: { // SUB
            int v = mem[op];
            std::cout << "ACC = " << acc << " - " << nomVar(op)
                      << "(" << v << ") = " << (acc - v) << "\n";
            acc -= v; pc += 2; break;
        }
        case 0x80: // JMP
            std::cout << "PC -> " << op << "\n";
            pc = op; break;
        case 0xA0: // JZ
            if (acc == 0) {
                std::cout << "ACC=0, PC -> " << op << "\n";
                pc = op;
            } else {
                std::cout << "ACC=" << acc << ", nao salta\n";
                pc += 2;
            }
            break;
        default:
            std::cout << "instrucao desconhecida (opc=" << opc << ")\n";
            return 1;
        }
    }

    std::cout << "\n--- Estado final das variaveis ---\n";
    for (auto& [a, n] : vars)
        if (n.rfind("CONST", 0) != 0)
            std::cout << n << " = " << mem[a] << "\n";
    std::cout << "\n";
    return 0;
}
)WNSIM";

        f.close();

        // Compila para Linux
        std::string cmdLinux = "g++ -std=c++17 -O2 -o " + nomeExec + " " + cppFile;
        if (system(cmdLinux.c_str()) == 0)
            std::cout << "Simulador Linux:   " << nomeExec << "\n";
        else
            std::cerr << "Aviso: falha ao compilar simulador Linux\n";

        // Compila para Windows (cross-compile via MinGW)
        std::string exeFile = nomeExec + ".exe";
        std::string cmdWin  = "x86_64-w64-mingw32-g++ -std=c++17 -O2 -static "
                              "-o " + exeFile + " " + cppFile;
        if (system(cmdWin.c_str()) == 0)
            std::cout << "Simulador Windows: " << exeFile << "\n";
        else
            std::cerr << "Aviso: MinGW nao encontrado, .exe nao gerado\n";

        std::remove(cppFile.c_str());
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
// SECAO 4: ANALISE SINTATICA + SEMANTICA (descida recursiva)
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

    void analisar() {
        parsePrograma();
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
    void parsePrograma() {
        consumir(TipoToken::BEGIN);
        parseListaCmds();
        consumir(TipoToken::END);
        gerador.emitir("HALT");

        if (atual().tipo != TipoToken::FIM)
            throw std::runtime_error(
                "Tokens inesperados apos END na linha " +
                std::to_string(atual().linha));
    }

    // lista_cmds -> cmd lista_cmds | vazio
    void parseListaCmds() {
        while (atual().tipo == TipoToken::IDENTIFICADOR ||
               atual().tipo == TipoToken::IF ||
               atual().tipo == TipoToken::WHILE) {
            parseCmd();
        }
    }

    // cmd -> atrib | se | enquanto
    void parseCmd() {
        if      (atual().tipo == TipoToken::IDENTIFICADOR) parseAtrib();
        else if (atual().tipo == TipoToken::IF)            parseSe();
        else if (atual().tipo == TipoToken::WHILE)         parseEnquanto();
        else
            throw std::runtime_error(
                "Erro sintatico na linha " + std::to_string(atual().linha) +
                ": comando invalido '" + atual().valor + "'");
    }

    // atrib -> ID = expr
    // expr  -> termo (('+' | '-') termo)*
    void parseAtrib() {
        Token dest = consumir(TipoToken::IDENTIFICADOR);
        consumir(TipoToken::ATRIBUICAO);

        // Carrega primeiro termo no acumulador
        gerador.emitir("LOAD", enderecoTermo());

        // Operacoes adicionais (soma ou subtracao)
        while (atual().tipo == TipoToken::MAIS ||
               atual().tipo == TipoToken::MENOS) {
            bool soma = (atual().tipo == TipoToken::MAIS);
            pos++;
            gerador.emitir(soma ? "ADD" : "SUB", enderecoTermo());
        }

        // Armazena resultado na variavel destino
        gerador.emitir("STORE", std::to_string(tabela.obterVar(dest.valor)));
    }

    // Retorna (como string) o endereco de memoria do proximo termo,
    // consumindo o token sem emitir instrucao de carga
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
    //
    // Codigo gerado:
    //     LOAD  <cond>
    //     JZ    ENDIF_n
    //     <corpo>
    // ENDIF_n:
    //     <proxima instrucao>
    void parseSe() {
        consumir(TipoToken::IF);
        Token cond = consumir(TipoToken::IDENTIFICADOR);
        consumir(TipoToken::THEN);

        std::string rotEndif = gerador.novoRotulo("ENDIF");

        gerador.emitir("LOAD", std::to_string(tabela.obterVar(cond.valor)));
        gerador.emitir("JZ",   rotEndif);

        parseListaCmds();

        consumir(TipoToken::ENDIF);

        // O proximo emit (do proprio corpo, proxima instrucao ou HALT) recebe o rotulo
        gerador.definirRotulo(rotEndif);
    }

    // enquanto -> WHILE ID DO lista_cmds ENDWHILE
    //
    // Codigo gerado:
    // WHILE_n:
    //     LOAD  <cond>
    //     JZ    ENDWHILE_m
    //     <corpo>
    //     JMP   WHILE_n
    // ENDWHILE_m:
    //     <proxima instrucao>
    void parseEnquanto() {
        consumir(TipoToken::WHILE);
        Token cond = consumir(TipoToken::IDENTIFICADOR);
        consumir(TipoToken::DO);

        std::string rotIni = gerador.novoRotulo("WHILE");
        std::string rotFim = gerador.novoRotulo("ENDWHILE");

        // LOAD recebe o rotulo de inicio do laco
        gerador.definirRotulo(rotIni);
        gerador.emitir("LOAD", std::to_string(tabela.obterVar(cond.valor)));
        gerador.emitir("JZ",   rotFim);

        parseListaCmds();

        consumir(TipoToken::ENDWHILE);

        gerador.emitir("JMP", rotIni);
        // A proxima instrucao recebe o rotulo de saida do laco
        gerador.definirRotulo(rotFim);
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
    std::cout << "    COMPILADOR HIPOTETICO -> WNEANDER\n";
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

        // FASES 2, 3 e 4: Sintatica + Semantica + Geracao de Codigo
        TabelaDeSimbolos tabela;
        GeradorDeCodigo  gerador;
        Parser parser(tokens, tabela, gerador);
        parser.analisar();

        tabela.imprimir();
        gerador.imprimir();
        gerador.imprimirImagemDeMemoria(tabela);
        gerador.exportarMemoria(arquivoSaida, tabela);
        gerador.gerarSimulador(nomeExec, tabela);

        std::cout << "\nCompilacao concluida com SUCESSO!\n";
        std::cout << "Execute './" << nomeExec << "' para ver a simulacao verbose.\n";
        std::cout << std::string(W, '=') << "\n\n";

    } catch (const std::exception& ex) {
        std::cerr << "\nERRO DE COMPILACAO: " << ex.what() << "\n";
        codigoRetorno = 1;
    }

    return codigoRetorno;
}
