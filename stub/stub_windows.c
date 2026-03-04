// ============================================================
// stub_windows.c
// Interpretador NEANDERWIN standalone — ZERO dependencia de CRT/MSVCRT
// Usa apenas kernel32.dll (Win32 API)
//
// Cross-compilacao (Linux → Windows via MinGW):
//   x86_64-w64-mingw32-gcc -nostdlib -Os -fno-stack-protector \
//       -fno-asynchronous-unwind-tables -fno-builtin \
//       -lkernel32 -o stub_windows.exe stub_windows.c
//
// Layout dos dados (appended pelo compilador no final do .exe):
//   [ultimos 1792 bytes do arquivo]
//     - VarEntry vars[32]  (768 bytes)
//     - int32 mem[256]     (1024 bytes)
// ============================================================

// ---------- Tipos ----------
typedef int            i32;
typedef unsigned int   u32;
typedef long long      i64;
typedef unsigned long long u64;
typedef void*          HANDLE;
typedef u32            DWORD;
typedef int            BOOL;

// ---------- Constantes Win32 ----------
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define INVALID_HANDLE    ((HANDLE)(i64)-1)
#define GENERIC_READ      0x80000000
#define OPEN_EXISTING     3
#define FILE_BEGIN        0
#define FILE_END          2
#define MAX_PATH          260

// ---------- Importacoes kernel32.dll ----------
__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD);
__declspec(dllimport) BOOL   __stdcall WriteFile(HANDLE, const void*, DWORD, DWORD*, void*);
__declspec(dllimport) DWORD  __stdcall GetModuleFileNameA(void*, char*, DWORD);
__declspec(dllimport) HANDLE __stdcall CreateFileA(const char*, DWORD, DWORD, void*, DWORD, DWORD, void*);
__declspec(dllimport) BOOL   __stdcall ReadFile(HANDLE, void*, DWORD, DWORD*, void*);
__declspec(dllimport) BOOL   __stdcall SetFilePointerEx(HANDLE, i64, i64*, DWORD);
__declspec(dllimport) BOOL   __stdcall CloseHandle(HANDLE);
__declspec(dllimport) void   __stdcall ExitProcess(u32);

// ---------- Handle stdout ----------
static HANDLE hStdout;

// ---------- Helpers de output ----------
static void w_out(const char* s, int len) {
    DWORD written;
    WriteFile(hStdout, s, (DWORD)len, &written, 0);
}

static void w_ch(char c) { w_out(&c, 1); }

static int i2s(int n, char* b) {
    if (n < 0) { b[0] = '-'; return 1 + i2s(-n, b+1); }
    if (n == 0) { b[0] = '0'; return 1; }
    char t[12]; int l = 0;
    while (n > 0) { t[l++] = '0' + (n % 10); n /= 10; }
    for (int i = 0; i < l; i++) b[i] = t[l-1-i];
    return l;
}

static void w_int(int n) {
    char b[16]; w_out(b, i2s(n, b));
}

static void w_int_w(int n, int w) {
    char b[16]; int l = i2s(n, b);
    w_out(b, l);
    for (int i = l; i < w; i++) w_ch(' ');
}

static int slen(const char* s) { int n=0; while(s[n]) n++; return n; }

static void w_str(const char* s) { w_out(s, slen(s)); }

static void w_str_w(const char* s, int sl, int w) {
    w_out(s, sl);
    for (int i = sl; i < w; i++) w_ch(' ');
}

// ---------- Dados NEANDERWIN ----------
#define MAX_VARS 32

typedef struct { i32 addr; i32 name_len; char name[16]; } VarEntry;

static i32      wn_mem[256];
static VarEntry wn_vars[MAX_VARS];

static void load_data(void) {
    // Descobre o caminho do proprio .exe
    char path[MAX_PATH];
    GetModuleFileNameA(0, path, MAX_PATH);

    // Abre o proprio arquivo
    HANDLE hFile = CreateFileA(path, GENERIC_READ, 1, 0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE) ExitProcess(1);

    // Busca ultimos 1792 bytes (768 + 1024)
    i64 offset = -(i64)(sizeof(wn_vars) + sizeof(wn_mem));
    SetFilePointerEx(hFile, offset, 0, FILE_END);

    DWORD bytesRead;
    ReadFile(hFile, wn_vars, sizeof(wn_vars), &bytesRead, 0);
    ReadFile(hFile, wn_mem,  sizeof(wn_mem),  &bytesRead, 0);

    CloseHandle(hFile);
}

// ---------- Nomes de variaveis ----------
static void w_var(int addr) {
    for (int i = 0; i < MAX_VARS && wn_vars[i].addr != -1; i++)
        if (wn_vars[i].addr == addr) {
            w_out(wn_vars[i].name, wn_vars[i].name_len);
            return;
        }
    w_str("MEM["); w_int(addr); w_ch(']');
}

// ---------- Nomes de instrucoes ----------
static void w_instr(int opc, int w) {
    const char* n; int l;
    switch(opc) {
        case 0x10: n="STORE"; l=5; break;
        case 0x20: n="LOAD";  l=4; break;
        case 0x30: n="ADD";   l=3; break;
        case 0x40: n="SUB";   l=3; break;
        case 0x80: n="JMP";   l=3; break;
        case 0xA0: n="JZ";    l=2; break;
        case 0xF0: n="HALT";  l=4; break;
        default:   n="???";   l=3; break;
    }
    w_str_w(n, l, w);
}

// ---------- Interpretador (entry point) ----------
void __stdcall mainCRTStartup(void) {
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    load_data();

    w_str("\n================================================\n"
          "   SIMULADOR NEANDERWIN - execucao verbose\n"
          "================================================\n\n"
          "PC    Instr   Op    ACC     Acao\n"
          "----------------------------------------------------\n");

    int acc = 0, pc = 0;

    for (int step = 0; step < 100000; step++) {
        int opc = wn_mem[pc];
        int hop = (opc != 0xF0);
        int op  = hop ? wn_mem[pc+1] : 0;

        w_int_w(pc, 6);
        w_instr(opc, 8);
        if (hop) w_int_w(op, 6); else w_out("      ", 6);
        w_int_w(acc, 8);

        if (opc == 0xF0) { w_str("HALT\n"); break; }

        switch(opc) {
        case 0x20: { // LOAD
            int v = wn_mem[op];
            w_str("ACC = "); w_var(op); w_str(" = ");
            acc = v; w_int(acc); w_ch('\n'); pc += 2; break;
        }
        case 0x10: { // STORE
            w_var(op); w_str(" = "); w_int(acc); w_ch('\n');
            wn_mem[op] = acc; pc += 2; break;
        }
        case 0x30: { // ADD
            int v = wn_mem[op];
            w_str("ACC = "); w_int(acc); w_str(" + ");
            w_var(op); w_ch('('); w_int(v); w_str(") = ");
            acc += v; w_int(acc); w_ch('\n'); pc += 2; break;
        }
        case 0x40: { // SUB
            int v = wn_mem[op];
            w_str("ACC = "); w_int(acc); w_str(" - ");
            w_var(op); w_ch('('); w_int(v); w_str(") = ");
            acc -= v; w_int(acc); w_ch('\n'); pc += 2; break;
        }
        case 0x80: // JMP
            w_str("PC -> "); w_int(op); w_ch('\n');
            pc = op; break;
        case 0xA0: // JZ
            if (acc == 0) {
                w_str("ACC=0, PC -> "); w_int(op); w_ch('\n');
                pc = op;
            } else {
                w_str("ACC="); w_int(acc);
                w_str(", nao salta\n"); pc += 2;
            }
            break;
        default:
            w_str("instrucao desconhecida\n"); ExitProcess(1);
        }
    }

    w_str("\n--- Estado final das variaveis ---\n");
    for (int i = 0; i < MAX_VARS && wn_vars[i].addr != -1; i++) {
        VarEntry* v = &wn_vars[i];
        if (v->name[0]=='C' && v->name[1]=='O' && v->name[2]=='N') continue;
        w_out(v->name, v->name_len);
        w_str(" = "); w_int(wn_mem[v->addr]); w_ch('\n');
    }
    w_ch('\n');
    ExitProcess(0);
}
