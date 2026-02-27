// ============================================================
// stub_linux.c
// Interpretador WNEANDER standalone — ZERO dependencia de libc
// Usa apenas syscalls Linux x86-64
//
// Compilacao:
//   gcc -nostdlib -static -Os -fno-stack-protector \
//       -fno-asynchronous-unwind-tables -no-pie -o stub_linux stub_linux.c
//
// Layout dos dados (appended pelo compilador no final do binario):
//   [ultimos 1792 bytes do arquivo]
//     - VarEntry vars[32]  (768 bytes)  ← lido primeiro
//     - int32 mem[256]     (1024 bytes) ← lido depois
// ============================================================

// ---------- Tipos ----------
typedef long           i64;
typedef int            i32;
typedef unsigned long  u64;

// ---------- Syscalls Linux x86-64 ----------
static inline i64 sc3(i64 n, i64 a, i64 b, i64 c) {
    i64 r;
    __asm__ volatile("syscall"
        : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c)
        : "rcx","r11","memory");
    return r;
}

static void   w_out(const char* s, i64 len) { sc3(1, 1, (i64)s, len); }
static void   w_ch(char c)                  { sc3(1, 1, (i64)&c, 1); }
static i64    f_read(int fd, void* b, i64 n){ return sc3(0, fd, (i64)b, n); }
static int    f_open(const char* p, int fl)  { return (int)sc3(2, (i64)p, fl, 0); }
static void   f_close(int fd)               { sc3(3, fd, 0, 0); }

static i64 f_lseek(int fd, i64 off, int wh) {
    return sc3(8, fd, off, (i64)wh);
}

static void die(int code) {
    sc3(231, code, 0, 0);
    __builtin_unreachable();
}

// ---------- Helpers de output ----------
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

// ---------- Dados WNEANDER (lidos do final do proprio binario) ----------
#define MAX_VARS 32

typedef struct { i32 addr; i32 name_len; char name[16]; } VarEntry;

static i32      wn_mem[256];
static VarEntry wn_vars[MAX_VARS];

static void load_data(void) {
    int fd = f_open("/proc/self/exe", 0);
    if (fd < 0) die(1);
    // Dados estao nos ultimos (768 + 1024) = 1792 bytes
    f_lseek(fd, -(i64)(sizeof(wn_vars) + sizeof(wn_mem)), 2);
    f_read(fd, wn_vars, sizeof(wn_vars));
    f_read(fd, wn_mem,  sizeof(wn_mem));
    f_close(fd);
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

// ---------- Interpretador ----------
void _start(void) {
    load_data();

    w_str("\n================================================\n"
          "   SIMULADOR WNEANDER - execucao verbose\n"
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
            w_str("instrucao desconhecida\n"); die(1);
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
    die(0);
}
