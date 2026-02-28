// ============================================================
// compilador.cpp
// Compilador Hipotetico para a Maquina WNEANDER
//
// Disciplina: Compiladores - Ciencia da Computacao
// Avaliacao: Trabalho Pratico (TB_PRATICO.pdf)
//
// Fases implementadas:
//   1. Analise Lexica
//   2. Analise Sintatica (descida recursiva)
//   3. Analise Semantica (tabela de simbolos)
//   4. Geracao de Codigo WNEANDER
//
// Compativel com Windows (MinGW/MSVC) e Linux (g++)
// ============================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cctype>
#include <iomanip>

// ============================================================
// SECAO 1 - ANALISE LEXICA
// ============================================================
//
// A analise lexica converte o codigo-fonte em uma sequencia de
// tokens. Tokens sao as unidades minimas com significado na
// linguagem (palavras reservadas, identificadores, numeros,
// operadores, etc.).
//
// Tokens reconhecidos:
//   Palavras reservadas: BEGIN, END, IF, THEN, ENDIF,
//                        WHILE, DO, ENDWHILE
//   Identificadores: sequencias de letras/digitos (ex: a, b, c)
//   Numeros inteiros: sequencias de digitos (ex: 3, 0)
//   Operadores: = (atribuicao), + (soma), - (subtracao)
// ============================================================

// Tipos de token da linguagem hipotetica
enum class TipoToken {
    // Palavras reservadas
    BEGIN, END,
    IF, THEN, ENDIF,
    WHILE, DO, ENDWHILE,
    // Literais
    IDENT,   // identificador (nome de variavel)
    NUM,     // numero inteiro
    // Operadores
    ATRIB,   // =
    MAIS,    // +
    MENOS,   // -
    // Fim de entrada
    FIM
};

// Converte tipo de token para texto legivel (usado em mensagens de erro)
std::string nomeToken(TipoToken t) {
    switch (t) {
        case TipoToken::BEGIN:    return "BEGIN";
        case TipoToken::END:      return "END";
        case TipoToken::IF:       return "IF";
        case TipoToken::THEN:     return "THEN";
        case TipoToken::ENDIF:    return "ENDIF";
        case TipoToken::WHILE:    return "WHILE";
        case TipoToken::DO:       return "DO";
        case TipoToken::ENDWHILE: return "ENDWHILE";
        case TipoToken::IDENT:    return "IDENTIFICADOR";
        case TipoToken::NUM:      return "NUMERO";
        case TipoToken::ATRIB:    return "ATRIBUICAO (=)";
        case TipoToken::MAIS:     return "MAIS (+)";
        case TipoToken::MENOS:    return "MENOS (-)";
        case TipoToken::FIM:      return "FIM";
    }
    return "?";
}

// Estrutura que representa um token com seu tipo, valor textual e linha
struct Token {
    TipoToken   tipo;
    std::string valor;
    int         linha;
};

// Analisador Lexico: percorre o codigo-fonte caractere por caractere
// e produz uma lista de tokens
class AnalisadorLexico {
public:
    explicit AnalisadorLexico(const std::string& fonte)
        : src(fonte), pos(0), linha(1) {}

    // Percorre todo o codigo-fonte e retorna a lista de tokens
    std::vector<Token> tokenizar() {
        std::vector<Token> tokens;

        while (pos < src.size()) {
            pularEspacos();
            if (pos >= src.size()) break;

            char c = src[pos];

            if (c == '\n') {
                // Contagem de linhas para mensagens de erro precisas
                linha++;
                pos++;
                continue;
            }

            if (std::isalpha(c) || c == '_') {
                // Identificador ou palavra reservada
                tokens.push_back(lerPalavra());
            } else if (std::isdigit(c)) {
                // Numero inteiro
                tokens.push_back(lerNumero());
            } else if (c == '=') {
                tokens.push_back({TipoToken::ATRIB,  "=", linha});
                pos++;
            } else if (c == '+') {
                tokens.push_back({TipoToken::MAIS,   "+", linha});
                pos++;
            } else if (c == '-') {
                tokens.push_back({TipoToken::MENOS,  "-", linha});
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
    std::string src;
    size_t      pos;
    int         linha;

    // Avanca sobre espacos, tabulacoes e retornos de carro (Windows)
    void pularEspacos() {
        while (pos < src.size() &&
               (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r'))
            pos++;
    }

    // Le uma sequencia alfanumerica e verifica se e palavra reservada
    Token lerPalavra() {
        size_t ini = pos;
        while (pos < src.size() && (std::isalnum(src[pos]) || src[pos] == '_'))
            pos++;
        std::string p = src.substr(ini, pos - ini);

        // Verificar palavras reservadas da linguagem hipotetica
        if (p == "BEGIN")    return {TipoToken::BEGIN,    p, linha};
        if (p == "END")      return {TipoToken::END,      p, linha};
        if (p == "IF")       return {TipoToken::IF,       p, linha};
        if (p == "THEN")     return {TipoToken::THEN,     p, linha};
        if (p == "ENDIF")    return {TipoToken::ENDIF,    p, linha};
        if (p == "WHILE")    return {TipoToken::WHILE,    p, linha};
        if (p == "DO")       return {TipoToken::DO,       p, linha};
        if (p == "ENDWHILE") return {TipoToken::ENDWHILE, p, linha};

        // Nao e palavra reservada: e um identificador (nome de variavel)
        return {TipoToken::IDENT, p, linha};
    }

    // Le uma sequencia de digitos e retorna como token NUM
    Token lerNumero() {
        size_t ini = pos;
        while (pos < src.size() && std::isdigit(src[pos]))
            pos++;
        return {TipoToken::NUM, src.substr(ini, pos - ini), linha};
    }
};

// ============================================================
// SECAO 2 - TABELA DE SIMBOLOS
// ============================================================
//
// A tabela de simbolos e construida durante a analise semantica.
// Ela associa cada variavel e constante a um endereco unico
// de memoria da WNEANDER.
//
// Layout de memoria adotado:
//   Enderecos  0-127 : area de codigo (instrucoes geradas)
//   Enderecos 128-199: area de variaveis
//   Enderecos 200-255: area de constantes literais
// ============================================================

class TabelaDeSimbolos {
public:
    TabelaDeSimbolos() : proxVar(128), proxConst(200) {}

    // Retorna o endereco de memoria da variavel.
    // Se a variavel ainda nao existe, aloca um novo endereco.
    int obterVar(const std::string& nome) {
        if (vars.find(nome) == vars.end())
            vars[nome] = proxVar++;
        return vars[nome];
    }

    // Retorna o endereco de memoria da constante inteira.
    // Se a constante ainda nao existe, aloca um novo endereco.
    int obterConst(int valor) {
        if (consts.find(valor) == consts.end())
            consts[valor] = proxConst++;
        return consts[valor];
    }

    // Verifica se uma variavel ja foi declarada (usada por variaveis)
    bool varExiste(const std::string& nome) const {
        return vars.find(nome) != vars.end();
    }

    // Exibe a tabela de simbolos no console
    void imprimir() const {
        const int W = 50;
        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "             TABELA DE SIMBOLOS\n";
        std::cout << std::string(W, '=') << "\n";
        std::cout << std::left
                  << std::setw(16) << "Nome"
                  << std::setw(16) << "Tipo"
                  << "Endereco\n";
        std::cout << std::string(W, '-') << "\n";

        for (const auto& par : vars)
            std::cout << std::setw(16) << par.first
                      << std::setw(16) << "variavel"
                      << par.second << "\n";

        for (const auto& par : consts)
            std::cout << std::setw(16) << ("CONST_" + std::to_string(par.first))
                      << std::setw(16) << "constante"
                      << par.second
                      << "  (valor=" << par.first << ")\n";

        std::cout << std::string(W, '=') << "\n";
    }

    const std::map<std::string, int>& getVars()   const { return vars; }
    const std::map<int, int>&         getConsts()  const { return consts; }

private:
    std::map<std::string, int> vars;    // nome  -> endereco
    std::map<int, int>         consts;  // valor -> endereco
    int proxVar;    // proximo endereco livre para variavel
    int proxConst;  // proximo endereco livre para constante
};

// ============================================================
// SECAO 3 - GERACAO DE CODIGO WNEANDER
// ============================================================
//
// O gerador produz instrucoes WNEANDER com rotulos simbolicos
// para controle de fluxo (IF e WHILE). Na segunda passagem
// (ao construir a imagem de memoria), os rotulos sao
// resolvidos para enderecos numericos.
//
// Conjunto de instrucoes WNEANDER utilizado:
//   LOAD X  - carrega memoria[X] no acumulador
//   STORE X - armazena acumulador em memoria[X]
//   ADD X   - acumulador += memoria[X]
//   SUB X   - acumulador -= memoria[X]
//   JZ L    - se acumulador == 0, pula para rotulo L
//   JMP L   - pula incondicionalmente para rotulo L
//   HALT    - encerra a execucao
//
// Opcodes numericos (hexadecimal):
//   LOAD=0x20, STORE=0x10, ADD=0x30, SUB=0x40
//   JMP=0x80,  JZ=0xA0,    HALT=0xF0
// ============================================================

// Representa uma instrucao WNEANDER com rotulo opcional
struct Instrucao {
    std::string rotulo;   // rotulo desta posicao (pode ser vazio)
    std::string mnem;     // mnemonico (LOAD, STORE, ADD, etc.)
    std::string operando; // operando: numero ou nome de rotulo
};

class GeradorDeCodigo {
public:
    GeradorDeCodigo() : contRotulos(0) {}

    // Cria e retorna um novo rotulo unico com prefixo dado
    std::string novoRotulo(const std::string& pref) {
        return pref + "_" + std::to_string(contRotulos++);
    }

    // Registra o rotulo que sera aplicado na proxima instrucao emitida
    void definirRotulo(const std::string& rot) {
        proxRotulo = rot;
    }

    // Emite uma instrucao (com o rotulo pendente, se houver)
    void emitir(const std::string& mnem, const std::string& op = "") {
        instrucoes.push_back({proxRotulo, mnem, op});
        proxRotulo = "";
    }

    // Exibe o codigo assembly gerado no console
    void imprimirAssembly() const {
        const int W = 50;
        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "        CODIGO ASSEMBLY WNEANDER GERADO\n";
        std::cout << std::string(W, '=') << "\n";

        for (const auto& i : instrucoes) {
            if (!i.rotulo.empty())
                std::cout << i.rotulo << ":\n";
            std::cout << "    " << std::left << std::setw(8) << i.mnem;
            if (!i.operando.empty())
                std::cout << i.operando;
            std::cout << "\n";
        }

        std::cout << std::string(W, '=') << "\n";
    }

    // Salva o codigo assembly em arquivo texto
    void salvarAssembly(const std::string& arq) const {
        std::ofstream f(arq);
        if (!f) {
            std::cerr << "Aviso: nao foi possivel criar '" << arq << "'\n";
            return;
        }
        f << "; Codigo Assembly WNEANDER gerado pelo compilador hipotetico\n\n";
        for (const auto& i : instrucoes) {
            if (!i.rotulo.empty())
                f << i.rotulo << ":\n";
            f << "    " << i.mnem;
            if (!i.operando.empty())
                f << " " << i.operando;
            f << "\n";
        }
        std::cout << "Assembly salvo em:  " << arq << "\n";
    }

    // Exibe a imagem numerica de memoria (instrucoes + dados) no console
    void imprimirMemoria(const TabelaDeSimbolos& tab) const {
        std::vector<int> mem = construirMemoria(tab);

        const int W = 52;
        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "           IMAGEM DE MEMORIA WNEANDER\n";
        std::cout << std::string(W, '=') << "\n";
        std::cout << std::left
                  << std::setw(12) << "Endereco"
                  << std::setw(18) << "Valor (decimal)"
                  << "Hex\n";
        std::cout << std::string(W, '-') << "\n";

        for (int i = 0; i < 256; i++) {
            if (mem[i] != 0) {
                std::cout << std::setw(12) << i
                          << std::setw(18) << mem[i]
                          << "0x" << std::uppercase << std::hex
                          << std::setw(2) << std::setfill('0') << mem[i]
                          << std::dec << std::setfill(' ') << "\n";
            }
        }

        std::cout << std::string(W, '=') << "\n";
    }

    // Salva a imagem de memoria no formato "endereco valor" (um por linha)
    // Este arquivo pode ser carregado diretamente em simuladores WNEANDER
    void salvarMemoria(const std::string& arq, const TabelaDeSimbolos& tab) const {
        std::vector<int> mem = construirMemoria(tab);
        std::ofstream f(arq);
        if (!f) {
            std::cerr << "Aviso: nao foi possivel criar '" << arq << "'\n";
            return;
        }
        for (int i = 0; i < 256; i++)
            f << i << " " << mem[i] << "\n";
        std::cout << "Memoria salva em:   " << arq << "\n";
    }

private:
    std::vector<Instrucao> instrucoes;
    std::string            proxRotulo;  // rotulo aguardando emissao
    int                    contRotulos;

    // Constroi o vetor de 256 posicoes representando a memoria WNEANDER
    // (instrucoes seguidas de dados de variaveis e constantes)
    std::vector<int> construirMemoria(const TabelaDeSimbolos& tab) const {
        // Opcodes da WNEANDER
        std::map<std::string, int> opcodes;
        opcodes["LOAD"]  = 0x20;
        opcodes["STORE"] = 0x10;
        opcodes["ADD"]   = 0x30;
        opcodes["SUB"]   = 0x40;
        opcodes["JMP"]   = 0x80;
        opcodes["JZ"]    = 0xA0;
        opcodes["HALT"]  = 0xF0;

        // 1a passagem: mapear nome de rotulo -> endereco de instrucao
        std::map<std::string, int> mapaRotulos;
        int addr = 0;
        for (const auto& i : instrucoes) {
            if (!i.rotulo.empty())
                mapaRotulos[i.rotulo] = addr;
            // HALT ocupa 1 byte; demais instrucoes ocupam 2 (opcode + operando)
            addr += (i.mnem == "HALT") ? 1 : 2;
        }

        // 2a passagem: preencher a memoria com os valores numericos
        std::vector<int> mem(256, 0);
        addr = 0;
        for (const auto& i : instrucoes) {
            mem[addr++] = opcodes[i.mnem];
            if (i.mnem != "HALT") {
                // Resolver operando: pode ser numero literal ou nome de rotulo
                int val = 0;
                bool convertido = false;
                try {
                    val = std::stoi(i.operando);
                    convertido = true;
                } catch (...) {}

                if (!convertido) {
                    if (mapaRotulos.count(i.operando))
                        val = mapaRotulos[i.operando];
                    else
                        throw std::runtime_error(
                            "Rotulo nao definido: '" + i.operando + "'");
                }
                mem[addr++] = val;
            }
        }

        // Inicializar area de dados: variaveis com 0
        for (const auto& par : tab.getVars())
            if (par.second < 256)
                mem[par.second] = 0;

        // Inicializar area de dados: constantes com seu valor real
        for (const auto& par : tab.getConsts())
            if (par.second < 256)
                mem[par.second] = par.first;

        return mem;
    }
};

// ============================================================
// SECAO 4 - ANALISE SINTATICA + SEMANTICA (descida recursiva)
// ============================================================
//
// O parser valida a estrutura sintatica do programa e, ao mesmo
// tempo, gera o codigo WNEANDER (analise guiada pela sintaxe).
//
// Gramatica da linguagem hipotetica (BNF simplificada):
//   programa  -> BEGIN lista END
//   lista     -> cmd lista | vazio
//   cmd       -> atrib | se | enquanto
//   atrib     -> IDENT = expr
//   expr      -> termo ( ('+' | '-') termo )*
//   termo     -> IDENT | NUM
//   se        -> IF IDENT THEN lista ENDIF
//   enquanto  -> WHILE IDENT DO lista ENDWHILE
//
// Analise semantica realizada:
//   - Toda variavel usada em leitura recebe endereco na tabela
//   - Toda variavel usada em escrita recebe endereco na tabela
//   - Constantes literais recebem endereco proprio na tabela
// ============================================================

class Parser {
public:
    Parser(const std::vector<Token>& toks,
           TabelaDeSimbolos&  tab,
           GeradorDeCodigo&   gen)
        : toks(toks), pos(0), tab(tab), gen(gen) {}

    void analisar() { parsePrograma(); }

private:
    std::vector<Token> toks;
    size_t             pos;
    TabelaDeSimbolos&  tab;
    GeradorDeCodigo&   gen;

    // Retorna o token atualmente apontado
    const Token& atual() const { return toks[pos]; }

    // Consome o token atual se for do tipo esperado; erro sintatico caso contrario
    Token consumir(TipoToken esp) {
        if (atual().tipo != esp)
            throw std::runtime_error(
                "Erro sintatico na linha " + std::to_string(atual().linha) +
                ": esperado '" + nomeToken(esp) +
                "', encontrado '" + atual().valor + "'");
        return toks[pos++];
    }

    // programa -> BEGIN lista END
    void parsePrograma() {
        consumir(TipoToken::BEGIN);
        parseLista();
        consumir(TipoToken::END);
        gen.emitir("HALT");  // fim do programa

        if (atual().tipo != TipoToken::FIM)
            throw std::runtime_error(
                "Tokens inesperados apos END na linha " +
                std::to_string(atual().linha));
    }

    // lista -> cmd lista | vazio
    // Continua enquanto o proximo token iniciar um comando valido
    void parseLista() {
        while (atual().tipo == TipoToken::IDENT  ||
               atual().tipo == TipoToken::IF     ||
               atual().tipo == TipoToken::WHILE)
            parseCmd();
    }

    // cmd -> atrib | se | enquanto
    void parseCmd() {
        if      (atual().tipo == TipoToken::IDENT)  parseAtrib();
        else if (atual().tipo == TipoToken::IF)      parseSe();
        else if (atual().tipo == TipoToken::WHILE)   parseEnquanto();
        else
            throw std::runtime_error(
                "Erro sintatico na linha " + std::to_string(atual().linha) +
                ": comando invalido '" + atual().valor + "'");
    }

    // atrib -> IDENT = expr
    // expr  -> termo ( ('+' | '-') termo )*
    //
    // Codigo gerado (ex: a = b + c - 1):
    //   LOAD  <addr_b>
    //   ADD   <addr_c>
    //   SUB   <addr_CONST_1>
    //   STORE <addr_a>
    void parseAtrib() {
        Token dest = consumir(TipoToken::IDENT);   // variavel destino
        consumir(TipoToken::ATRIB);

        // Carrega o primeiro termo no acumulador
        gen.emitir("LOAD", enderecoTermo());

        // Processa operacoes seguintes (+ ou -)
        while (atual().tipo == TipoToken::MAIS ||
               atual().tipo == TipoToken::MENOS) {
            bool soma = (atual().tipo == TipoToken::MAIS);
            pos++;
            gen.emitir(soma ? "ADD" : "SUB", enderecoTermo());
        }

        // Armazena o resultado na variavel destino
        gen.emitir("STORE", std::to_string(tab.obterVar(dest.valor)));
    }

    // Retorna o endereco de memoria do proximo termo (IDENT ou NUM)
    // sem emitir instrucao de carga (o chamador decide qual instrucao usar)
    std::string enderecoTermo() {
        if (atual().tipo == TipoToken::IDENT) {
            Token t = consumir(TipoToken::IDENT);
            return std::to_string(tab.obterVar(t.valor));
        }
        if (atual().tipo == TipoToken::NUM) {
            Token t = consumir(TipoToken::NUM);
            return std::to_string(tab.obterConst(std::stoi(t.valor)));
        }
        throw std::runtime_error(
            "Esperado identificador ou numero na linha " +
            std::to_string(atual().linha) +
            ", encontrado '" + atual().valor + "'");
    }

    // se -> IF IDENT THEN lista ENDIF
    //
    // A condicao e verdadeira se o valor da variavel for DIFERENTE de zero.
    // Codigo gerado:
    //   LOAD  <addr_cond>
    //   JZ    ENDIF_n          ; pula o corpo se cond == 0
    //   <corpo do IF>
    // ENDIF_n:
    //   <proxima instrucao>
    void parseSe() {
        consumir(TipoToken::IF);
        Token cond = consumir(TipoToken::IDENT);   // variavel condicao
        consumir(TipoToken::THEN);

        std::string rotFim = gen.novoRotulo("ENDIF");

        gen.emitir("LOAD", std::to_string(tab.obterVar(cond.valor)));
        gen.emitir("JZ",   rotFim);  // se zero, pula o corpo

        parseLista();
        consumir(TipoToken::ENDIF);

        // O rotulo e aplicado na proxima instrucao emitida
        gen.definirRotulo(rotFim);
    }

    // enquanto -> WHILE IDENT DO lista ENDWHILE
    //
    // A condicao e verdadeira se o valor da variavel for DIFERENTE de zero.
    // Codigo gerado:
    // WHILE_n:
    //   LOAD  <addr_cond>
    //   JZ    ENDWHILE_m       ; sai do laco se cond == 0
    //   <corpo do WHILE>
    //   JMP   WHILE_n          ; volta ao inicio do laco
    // ENDWHILE_m:
    //   <proxima instrucao>
    void parseEnquanto() {
        consumir(TipoToken::WHILE);
        Token cond = consumir(TipoToken::IDENT);   // variavel condicao
        consumir(TipoToken::DO);

        std::string rotIni = gen.novoRotulo("WHILE");
        std::string rotFim = gen.novoRotulo("ENDWHILE");

        // Rotulo de inicio do laco (alvo do JMP de retorno)
        gen.definirRotulo(rotIni);
        gen.emitir("LOAD", std::to_string(tab.obterVar(cond.valor)));
        gen.emitir("JZ",   rotFim);

        parseLista();
        consumir(TipoToken::ENDWHILE);

        gen.emitir("JMP", rotIni);           // volta ao inicio
        gen.definirRotulo(rotFim);           // rotulo de saida do laco
    }
};

// ============================================================
// SECAO 5 - PROGRAMA PRINCIPAL
// ============================================================

// Exemplo de codigo-fonte extraido diretamente do enunciado (PDF)
static const std::string EXEMPLO_PDF =
    "BEGIN\n"
    "a=3\n"
    "b=0\n"
    "WHILE a DO\n"
    "b=b+a\n"
    "a=a-1\n"
    "ENDWHILE\n"
    "IF b THEN\n"
    "c=b\n"
    "ENDIF\n"
    "END\n";

// Extrai o nome base de um caminho de arquivo (sem extensao)
// Ex: "C:\\prog\\meu_prog.wn" -> "meu_prog"
static std::string nomeBase(const std::string& caminho) {
    // Localizar ultima barra (Linux ou Windows)
    size_t barra = caminho.find_last_of("/\\");
    std::string nome = (barra == std::string::npos)
                       ? caminho
                       : caminho.substr(barra + 1);
    // Remover extensao
    size_t ponto = nome.rfind('.');
    if (ponto != std::string::npos)
        nome = nome.substr(0, ponto);
    return nome.empty() ? "saida" : nome;
}

int main(int argc, char* argv[]) {
    std::string fonte;
    std::string nomeArq;
    std::string base;   // nome base para arquivos de saida

    if (argc >= 2) {
        // Modo arquivo: compila o .wn passado como argumento
        nomeArq = argv[1];
        std::ifstream f(nomeArq.c_str());
        if (!f) {
            std::cerr << "Erro: nao foi possivel abrir '" << nomeArq << "'\n";
            return 1;
        }
        std::stringstream ss;
        ss << f.rdbuf();
        fonte = ss.str();
        base  = nomeBase(nomeArq);
    } else {
        // Modo demo: compila o exemplo do enunciado embutido
        fonte  = EXEMPLO_PDF;
        nomeArq = "<exemplo do enunciado PDF>";
        base    = "saida";
    }

    const int W = 52;
    std::cout << "\n" << std::string(W, '=') << "\n";
    std::cout << "   COMPILADOR HIPOTETICO -> MAQUINA WNEANDER\n";
    std::cout << "   Disciplina: Compiladores\n";
    std::cout << std::string(W, '=') << "\n";

    std::cout << "\n--- CODIGO FONTE (" << nomeArq << ") ---\n";
    std::cout << fonte << "\n";

    int ret = 0;

    try {
        // --------------------------------------------------
        // FASE 1: ANALISE LEXICA
        // --------------------------------------------------
        AnalisadorLexico lexer(fonte);
        std::vector<Token> tokens = lexer.tokenizar();

        std::cout << "\n" << std::string(W, '=') << "\n";
        std::cout << "           TOKENS (ANALISE LEXICA)\n";
        std::cout << std::string(W, '=') << "\n";
        std::cout << std::left
                  << std::setw(8)  << "Linha"
                  << std::setw(16) << "Tipo"
                  << "Valor\n";
        std::cout << std::string(W, '-') << "\n";
        for (const auto& t : tokens) {
            if (t.tipo == TipoToken::FIM) break;
            std::cout << std::setw(8)  << t.linha
                      << std::setw(16) << nomeToken(t.tipo)
                      << t.valor << "\n";
        }
        std::cout << std::string(W, '=') << "\n";

        // --------------------------------------------------
        // FASES 2, 3 e 4: SINTATICA + SEMANTICA + GERACAO
        // --------------------------------------------------
        TabelaDeSimbolos tab;
        GeradorDeCodigo  gen;
        Parser parser(tokens, tab, gen);
        parser.analisar();

        // Exibir resultados no console
        tab.imprimir();
        gen.imprimirAssembly();
        gen.imprimirMemoria(tab);

        // Salvar arquivos de saida
        gen.salvarAssembly(base + ".asm");
        gen.salvarMemoria(base + ".mem", tab);

        std::cout << "\nCompilacao concluida com SUCESSO!\n";
        std::cout << std::string(W, '=') << "\n\n";

    } catch (const std::exception& e) {
        std::cerr << "\nERRO DE COMPILACAO: " << e.what() << "\n";
        ret = 1;
    }

    return ret;
}
