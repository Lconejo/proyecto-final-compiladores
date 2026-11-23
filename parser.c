/* ============================================================
 * parser.c - Analizador sintactico descendente recursivo (lenguaje X)
 *
 * Una funcion por cada no terminal de la gramatica LL(1) (42 en total).
 * Cada funcion decide con UN atomo de preanalisis (conjuntos de seleccion
 * del documento de la Parte 1) y consume los atomos que espera.
 *
 * Recuperacion de errores (modo panico sencillo):
 *   - emparejar() NO consume si el atomo no coincide: solo reporta (insercion virtual).
 *   - Tras un error se activa 'panico' y se suprimen reportes hasta consumir
 *     correctamente un atomo (evita errores en cascada).
 *   - Los lugares donde se "salta" texto (listaS, otraFunc, Program, F, ...)
 *     siempre consumen al menos un atomo o se detienen en el fin de archivo '$'
 *     => no hay ciclos infinitos.
 * ============================================================ */
#include <stdio.h>
#include <string.h>
#include "atomos.h"

/* ---------- Estado del analizador ---------- */
static int pos = 0;             /* indice del atomo de preanalisis */
static int erroresSint = 0;
static int panico = 0;

#define ACT()   (listaAtomos[pos].atomo)

/* En la tabla de atomos '&' es el operador de concatenacion de cadenas.
 * (La gramatica original usaba 'h' en <opCad>, que es el atomo de &&; ver documento.) */
#define ATOMO_CONCAT '&'

/* Conjuntos de seleccion / sincronizacion usados con en() */
#define PRIM_FUNC   "fgi"
#define PRIM_SENT   "[abpqstuz"          /* a=asig, [=llamada, b=trocar, p=para, q=enq, s=se, t=ret, u=quebrar, z=faz */
#define PRIM_EXPR   "([an"               /* First(E) */
#define SYNC_EXPR   ")%*+,-/;<>edhjmw}#]"

static int en(char c, const char *conjunto) { return c != '\0' && strchr(conjunto, c) != NULL; }

/* ---------- Prototipos: un no terminal = una funcion ---------- */
static void Program(void);   static void otraFunc(void);  static void Func(void);
static void TipoF(void);     static void listArg(void);   static void otroArg(void);
static void Cuerpo(void);    static void listDecl(void);  static void Decl(void);
static void Tipo(void);      static void valorIni(void);  static void listaVar(void);
static void tipoVal(void);   static void Asig(void);      static void opArit(void);
static void E(void);         static void Ep(void);        static void T(void);
static void Tp(void);        static void F(void);         static void R(void);
static void opRel(void);     static void Sent(void);      static void listaS(void);
static void expLogica(void); static void expLog(void);    static void expRel(void);
static void opLog(void);     static void Enq(void);       static void Se(void);
static void Q(void);         static void Faz(void);       static void Ret(void);
static void valRet(void);    static void expCad(void);    static void opCad(void);
static void Para(void);      static void Trocar(void);    static void casos(void);
static void Llama(void);     static void listP(void);     static void Param(void);

/* ---------- Utilidades ---------- */
static const char *nombreAtomo(char c) {
    static char buf[8];
    switch (c) {
        case 'a': return "identificador";        case 'n': return "constante numerica";
        case 'v': return "constante cadena";     case 'f': return "'flutuador'";
        case 'i': return "'inteiro'";            case 'g': return "'assinado'";
        case 'c': return "'caso'";               case 'q': return "'enquanto'";
        case 'z': return "'fazer'";              case 'p': return "'para'";
        case 'u': return "'quebrar'";            case 't': return "'retorno'";
        case 's': return "'se'";                 case 'b': return "'trocar'";
        case 'l': return "'like'";               case 'h': return "'&&'";
        case 'j': return "'||'";                 case 'e': return "'=='";
        case 'd': return "'!='";                 case 'm': return "'>='";
        case 'w': return "'<='";                 case 'x': return "'+='";
        case 'y': return "'-='";                 case 'k': return "'*='";
        case 'r': return "'/='";                 case 'o': return "'%='";
        case ATOMO_FIN: return "fin de archivo";
        default: buf[0] = '\''; buf[1] = c; buf[2] = '\''; buf[3] = '\0'; return buf;
    }
}

/* Reporta "se esperaba X pero se encontro Y" (con supresion en modo panico) */
static void errorEsperaba(const char *esperado) {
    if (panico) return;
    panico = 1;
    erroresSint++;
    Atomo *a = &listaAtomos[pos];
    if (a->atomo == ATOMO_FIN)
        printf("[Error Sintactico] Linea %d, Columna %d: se esperaba %s pero se llego al fin de archivo\n",
               a->linea, a->columna, esperado);
    else
        printf("[Error Sintactico] Linea %d, Columna %d: se esperaba %s pero se encontro %s ('%s')\n",
               a->linea, a->columna, esperado, nombreAtomo(a->atomo), a->lexema);
}

static void saltar(void) { if (ACT() != ATOMO_FIN) pos++; }   /* descarta, sin salir de panico */
static void sincronizar(const char *conjunto) {
    while (ACT() != ATOMO_FIN && !en(ACT(), conjunto)) saltar();
}

/* Consume el atomo esperado; si no coincide reporta y NO consume */
static void emparejar(char esperado) {
    if (ACT() == esperado) {
        if (esperado != ATOMO_FIN) pos++;
        panico = 0;
    } else {
        char desc[40];
        snprintf(desc, sizeof desc, "%s", nombreAtomo(esperado));
        errorEsperaba(desc);
    }
}

/* ============================================================
 *  Estructura del programa
 * ============================================================ */
/* 1: <Program> -> <Func><otraFunc> */
static void Program(void) {
    if (!en(ACT(), PRIM_FUNC)) {
        errorEsperaba("una funcion (tipo de retorno 'flutuador', 'inteiro' o 'assinado')");
        sincronizar(PRIM_FUNC);
    }
    if (ACT() != ATOMO_FIN) { Func(); otraFunc(); }
}

/* 2,3: <otraFunc> -> <Func><otraFunc> | e */
static void otraFunc(void) {
    if (en(ACT(), PRIM_FUNC))      { Func(); otraFunc(); }
    else if (ACT() == ATOMO_FIN)   { /* epsilon */ }
    else {
        errorEsperaba("otra funcion o el fin de archivo");
        saltar();                  /* siempre avanza */
        otraFunc();
    }
}

/* 4: <Func> -> <TipoF> a ( <listArg> ) { <Cuerpo> } */
static void Func(void) {
    TipoF(); emparejar('a'); emparejar('(');
    listArg();
    emparejar(')'); emparejar('{');
    Cuerpo();
    emparejar('}');
}

/* 5,6: <TipoF> -> <Tipo> | g */
static void TipoF(void) {
    if (en(ACT(), "fi")) Tipo();
    else if (ACT() == 'g') emparejar('g');
    else { errorEsperaba("un tipo de retorno"); sincronizar("a({"); }
}

/* 7,8: <listArg> -> <Tipo> a <otroArg> | e */
static void listArg(void) {
    if (en(ACT(), "fi")) { Tipo(); emparejar('a'); otroArg(); }
    /* epsilon por omision; un error se detecta en emparejar(')') */
}

/* 9,10: <otroArg> -> , <Tipo> a <otroArg> | e */
static void otroArg(void) {
    if (ACT() == ',') {
        emparejar(','); Tipo(); emparejar('a'); otroArg();
    }
}

/* 11: <Cuerpo> -> <listDecl> <listaS> */
static void Cuerpo(void) { listDecl(); listaS(); }

/* 12,13: <listDecl> -> e | <Decl><listDecl> */
static void listDecl(void) {
    if (en(ACT(), "fi")) { Decl(); listDecl(); }
}

/* ============================================================
 *  Declaraciones
 * ============================================================ */
/* 14: <Decl> -> <Tipo> a <valorIni> <listaVar> ; */
static void Decl(void) {
    Tipo(); emparejar('a'); valorIni(); listaVar(); emparejar(';');
}

/* 15,16: <Tipo> -> f | i */
static void Tipo(void) {
    if (ACT() == 'f') emparejar('f');
    else if (ACT() == 'i') emparejar('i');
    else { errorEsperaba("un tipo ('flutuador' o 'inteiro')"); sincronizar("a;"); }
}

/* 17,18: <valorIni> -> = <tipoVal> | e */
static void valorIni(void) {
    if (ACT() == '=') { emparejar('='); tipoVal(); }
}

/* 19,20: <listaVar> -> , a <valorIni> <listaVar> | e */
static void listaVar(void) {
    if (ACT() == ',') { emparejar(','); emparejar('a'); valorIni(); listaVar(); }
}

/* 21,22: <tipoVal> -> n | v */
static void tipoVal(void) {
    if (ACT() == 'n') emparejar('n');
    else if (ACT() == 'v') emparejar('v');
    else { errorEsperaba("una constante numerica o una cadena"); sincronizar(",;"); }
}

/* ============================================================
 *  Asignacion y expresiones aritmeticas
 * ============================================================ */
/* 23: <Asig> -> a <opArit> E ; */
static void Asig(void) {
    emparejar('a'); opArit(); E(); emparejar(';');
}

/* 24-29: <opArit> -> = | x | y | k | r | o */
static void opArit(void) {
    if (en(ACT(), "=xykro")) emparejar(ACT());
    else { errorEsperaba("un operador de asignacion (=, +=, -=, *=, /=, %=)"); sincronizar(PRIM_EXPR ";"); }
}

/* 30: E -> T E' */
static void E(void)  { T(); Ep(); }

/* 31-33: E' -> + T E' | - T E' | e */
static void Ep(void) {
    if (ACT() == '+')      { emparejar('+'); T(); Ep(); }
    else if (ACT() == '-') { emparejar('-'); T(); Ep(); }
}

/* 34: T -> F T' */
static void T(void)  { F(); Tp(); }

/* 35-38: T' -> * F T' | / F T' | % F T' | e */
static void Tp(void) {
    if (ACT() == '*')      { emparejar('*'); F(); Tp(); }
    else if (ACT() == '/') { emparejar('/'); F(); Tp(); }
    else if (ACT() == '%') { emparejar('%'); F(); Tp(); }
}

/* 39-42: F -> ( E ) | a | n | <Llama> */
static void F(void) {
    switch (ACT()) {
        case '(': emparejar('('); E(); emparejar(')'); break;
        case 'a': emparejar('a'); break;
        case 'n': emparejar('n'); break;
        case '[': Llama(); break;
        default:
            errorEsperaba("un operando (identificador, numero, '(' o llamada a funcion)");
            sincronizar(SYNC_EXPR);
    }
}

/* ============================================================
 *  Expresiones relacionales y logicas
 * ============================================================ */
/* 43: R -> E <opRel> E */
static void R(void) { E(); opRel(); E(); }

/* 44-49: <opRel> -> > | < | e | d | m | w */
static void opRel(void) {
    if (en(ACT(), "<>edmw")) emparejar(ACT());
    else errorEsperaba("un operador relacional (==, !=, >, <, >=, <=)");
}

/* 61,62: <expLogica> -> ! <expRel> | R <expLog> */
static void expLogica(void) {
    if (ACT() == '!')                { emparejar('!'); expRel(); }
    else if (en(ACT(), PRIM_EXPR))   { R(); expLog(); }
    else { errorEsperaba("una expresion logica"); sincronizar(")#}"); }
}

/* 63,64: <expLog> -> <opLog> R | e */
static void expLog(void) {
    if (en(ACT(), "hj")) { opLog(); R(); }
}

/* 65,66: <expRel> -> { R } | E */
static void expRel(void) {
    if (ACT() == '{')              { emparejar('{'); R(); emparejar('}'); }
    else if (en(ACT(), PRIM_EXPR)) { E(); }
    else { errorEsperaba("una expresion relacional ('{ ... }') o aritmetica"); sincronizar(")#}"); }
}

/* 67,68: <opLog> -> h | j */
static void opLog(void) {
    if (en(ACT(), "hj")) emparejar(ACT());
    else errorEsperaba("un operador logico (&& o ||)");
}

/* ============================================================
 *  Sentencias
 * ============================================================ */
/* 50-58: <Sent> -> <Asig> | Q | <Ret> | <Trocar> | <Enq> | <Faz> | <Se> | <Para> | <Llama> */
static void Sent(void) {
    switch (ACT()) {
        case 'a': Asig();   break;
        case 'u': Q();      break;
        case 't': Ret();    break;
        case 'b': Trocar(); break;
        case 'q': Enq();    break;
        case 'z': Faz();    break;
        case 's': Se();     break;
        case 'p': Para();   break;
        case '[': Llama();  break;
        default:  errorEsperaba("una sentencia"); saltar();
    }
}

/* 59,60: <listaS> -> <Sent> <listaS> | e      (epsilon: # } $) */
static void listaS(void) {
    if (en(ACT(), PRIM_SENT)) { Sent(); listaS(); }
    else if (en(ACT(), "#}") || ACT() == ATOMO_FIN) { /* epsilon */ }
    else {
        if (en(ACT(), "fi")) {
            /* Declaracion despues de sentencias: se reporta y se analiza igual
               (Decl consume al menos el tipo) para no generar errores en cascada */
            errorEsperaba("una sentencia (las declaraciones deben ir antes de las sentencias)");
            Decl();
        } else {
            errorEsperaba("una sentencia, '#' o '}'");
            saltar();              /* siempre avanza */
        }
        listaS();
    }
}

/* 69: <Enq> -> q ( <expLogica> ) # <listaS> # */
static void Enq(void) {
    emparejar('q'); emparejar('('); expLogica(); emparejar(')');
    emparejar('#'); listaS(); emparejar('#');
}

/* 70: <Se> -> s ( <expLogica> ) # <listaS> # */
static void Se(void) {
    emparejar('s'); emparejar('('); expLogica(); emparejar(')');
    emparejar('#'); listaS(); emparejar('#');
}

/* 71: Q -> u ; */
static void Q(void) { emparejar('u'); emparejar(';'); }

/* 72: <Faz> -> z # <listaS> # q ( <expLogica> ) ; */
static void Faz(void) {
    emparejar('z'); emparejar('#'); listaS(); emparejar('#');
    emparejar('q'); emparejar('('); expLogica(); emparejar(')'); emparejar(';');
}

/* 73: <Ret> -> t <valRet> ; */
static void Ret(void) { emparejar('t'); valRet(); emparejar(';'); }

/* 74-76: <valRet> -> E | { <expCad> } | e */
static void valRet(void) {
    if (en(ACT(), PRIM_EXPR)) E();
    else if (ACT() == '{')    { emparejar('{'); expCad(); emparejar('}'); }
}

/* 77,78: <expCad> -> a <opCad> | v <opCad> */
static void expCad(void) {
    if (ACT() == 'a')      { emparejar('a'); opCad(); }
    else if (ACT() == 'v') { emparejar('v'); opCad(); }
    else { errorEsperaba("un identificador o una cadena"); sincronizar("}"); }
}

/* 79-81: <opCad> -> l <expCad> | & <expCad> | e */
static void opCad(void) {
    if (ACT() == 'l')                { emparejar('l'); expCad(); }
    else if (ACT() == ATOMO_CONCAT)  { emparejar(ATOMO_CONCAT); expCad(); }
}

/* 82: <Para> -> p a [ n , n ] # <listaS> # */
static void Para(void) {
    emparejar('p'); emparejar('a'); emparejar('[');
    emparejar('n'); emparejar(','); emparejar('n');
    emparejar(']'); emparejar('#'); listaS(); emparejar('#');
}

/* 83: <Trocar> -> b ( a ) : # c ( n ) { <listaS> } <casos> # */
static void Trocar(void) {
    emparejar('b'); emparejar('('); emparejar('a'); emparejar(')'); emparejar(':');
    emparejar('#');
    emparejar('c'); emparejar('('); emparejar('n'); emparejar(')');
    emparejar('{'); listaS(); emparejar('}');
    casos();
    emparejar('#');
}

/* 84-86: <casos> -> e | c ( n ) { <listaS> } <casos> | ( ) { <listaS> } <casos> */
static void casos(void) {
    if (ACT() == 'c') {
        emparejar('c'); emparejar('('); emparejar('n'); emparejar(')');
        emparejar('{'); listaS(); emparejar('}'); casos();
    } else if (ACT() == '(') {
        emparejar('('); emparejar(')');
        emparejar('{'); listaS(); emparejar('}'); casos();
    }
}

/* 87: <Llama> -> [ a ( <listP> ) ] */
static void Llama(void) {
    emparejar('['); emparejar('a'); emparejar('(');
    listP();
    emparejar(')'); emparejar(']');
}

/* 88-90: <listP> -> e | E <Param> | { <expCad> } <Param> */
static void listP(void) {
    if (en(ACT(), PRIM_EXPR)) { E(); Param(); }
    else if (ACT() == '{')    { emparejar('{'); expCad(); emparejar('}'); Param(); }
}

/* 91,92: <Param> -> , <listP> | e */
static void Param(void) {
    if (ACT() == ',') { emparejar(','); listP(); }
}

/*Inicio*/
int analizadorSintactico(void) {
    pos = 0; erroresSint = 0; panico = 0;
    Program();
    return erroresSint;
}
