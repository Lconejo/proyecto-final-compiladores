/* ============================================================
 * atomos.c - Tablas del lexico, secuencia de atomos y reportes
 * ============================================================ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atomos.h"

Atomo listaAtomos[MAX_ATOMOS];
int   numAtomos      = 0;
int   erroresLexicos = 0;

static int linea_actual   = 1;
static int columna_actual = 1;
int tok_linea = 1, tok_col = 1;

/* --------------------------------------------------
 * Seguimiento de linea/columna. Se invoca ANTES de cada accion de Flex
 * (via YY_USER_ACTION): guarda donde INICIA el lexema y avanza el cursor.
 * -------------------------------------------------- */
void actualizarPosicion(const char *texto, int longitud) {
    tok_linea = linea_actual;
    tok_col   = columna_actual;
    for (int i = 0; i < longitud; i++) {
        if (texto[i] == '\n') { linea_actual++; columna_actual = 1; }
        else                  { columna_actual++; }
    }
}

/* --------------------------------------------------
 * TABLA DE SIMBOLOS (Identificadores - Clase 5)
 * -------------------------------------------------- */
typedef struct {
    int  posicion;
    char nombre[50];
    int  tipo;
} Simbolo;

static Simbolo tablaIden[150];
static int numIden = 0;

int guardarIden(const char *nombre) {
    for (int i = 0; i < numIden; i++)
        if (strcmp(tablaIden[i].nombre, nombre) == 0)
            return tablaIden[i].posicion;

    if (numIden >= 150) { fprintf(stderr, "Tabla de simbolos llena.\n"); exit(1); }
    tablaIden[numIden].posicion = numIden;
    strncpy(tablaIden[numIden].nombre, nombre, 49);
    tablaIden[numIden].nombre[49] = '\0';
    tablaIden[numIden].tipo = -1;
    numIden++;
    return numIden - 1;
}

/* --------------------------------------------------
 * TABLA DE LITERALES (Cadenas - Clase 8)
 * -------------------------------------------------- */
typedef struct {
    int  posicion;
    char dato[200];
} Literal;

static Literal tablaLiterales[150];
static int numLiterales = 0;

int guardarLiteral(const char *cadena) {
    if (numLiterales >= 150) { fprintf(stderr, "Tabla de literales llena.\n"); exit(1); }
    tablaLiterales[numLiterales].posicion = numLiterales;
    strncpy(tablaLiterales[numLiterales].dato, cadena, 199);
    tablaLiterales[numLiterales].dato[199] = '\0';
    numLiterales++;
    return numLiterales - 1;
}

/* --------------------------------------------------
 * Emision de tokens y atomos
 * -------------------------------------------------- */
static void guardarAtomo(char atomo, int clase, int valor, const char *lexema) {
    if (numAtomos >= MAX_ATOMOS) {
        fprintf(stderr, "Demasiados atomos (maximo %d).\n", MAX_ATOMOS);
        exit(1);
    }
    Atomo *a = &listaAtomos[numAtomos++];
    a->atomo   = atomo;
    a->clase   = clase;
    a->valor   = valor;
    a->linea   = tok_linea;
    a->columna = tok_col;
    strncpy(a->lexema, lexema, MAX_LEXEMA - 1);
    a->lexema[MAX_LEXEMA - 1] = '\0';
}

/* Imprime el token <clase, valor> (reporte del Proyecto 1) y guarda el atomo */
void emitirToken(int clase, int valor, char atomo, const char *lexema) {
    printf("<%d, %d>\n", clase, valor);
    guardarAtomo(atomo, clase, valor, lexema);
}

void reportarErrorLexico(const char *lexema) {
    erroresLexicos++;
    printf("[Error Lexico] Linea %d, Columna %d: Simbolo no reconocido '%s'\n",
           tok_linea, tok_col, lexema);
}

/* --------------------------------------------------
 * Reportes
 * -------------------------------------------------- */
void imprimirEncabezado(void) {
    printf("======================================================\n");
    printf("   PROYECTO 2 (PARTE 2) - ANALIZADOR LEXICO Y SINTACTICO\n");
    printf("  Integrantes:\n");
    printf("  - Ambrosio Escobar Dominic Yael\n");
    printf("  - Briones Gomez Oliver Cesar Augusto\n");
    printf("  - Ramirez Rodriguez Oscar Gael\n");
    printf("  Fecha: Noviembre 2026\n");
    printf("======================================================\n\n");
}

static void imprimirTablas(void) {
    printf("\n\n--- TABLA DE SIMBOLOS (Identificadores) ---\n");
    printf("%-10s | %-25s | %-10s\n", "Posicion", "Nombre", "Tipo");
    printf("------------------------------------------------------\n");
    for (int i = 0; i < numIden; i++)
        printf("%-10d | %-25s | %-10d\n", tablaIden[i].posicion, tablaIden[i].nombre, tablaIden[i].tipo);

    printf("\n\n--- TABLA DE LITERALES (Cadenas) ---\n");
    printf("%-10s | %-40s\n", "Posicion", "Dato");
    printf("------------------------------------------------------\n");
    for (int i = 0; i < numLiterales; i++)
        printf("%-10d | %-40s\n", tablaLiterales[i].posicion, tablaLiterales[i].dato);
}

static void imprimirAtomos(void) {
    printf("\n\n--- SECUENCIA DE ATOMOS (entrada del analizador sintactico) ---\n");
    printf("%-5s | %-5s | %-9s | %-6s | %s\n", "No.", "Atomo", "Lin:Col", "Clase", "Lexema");
    printf("------------------------------------------------------\n");
    for (int i = 0; i < numAtomos; i++) {
        char pos[24];
        snprintf(pos, sizeof pos, "%d:%d", listaAtomos[i].linea, listaAtomos[i].columna);
        if (listaAtomos[i].atomo == ATOMO_FIN)
            printf("%-5d | %-5c | %-9s | %-6s | %s\n", i, listaAtomos[i].atomo, pos, "-", "(fin de archivo)");
        else
            printf("%-5d | %-5c | %-9s | %-6d | %s\n", i, listaAtomos[i].atomo, pos,
                   listaAtomos[i].clase, listaAtomos[i].lexema);
    }
    printf("\nCadena de atomos: ");
    for (int i = 0; i < numAtomos; i++) putchar(listaAtomos[i].atomo);
    printf("\n");
}

/* Se llama al terminar yylex(): cierra la secuencia y decide si hay analisis sintactico */
int finalizarAnalisis(void) {
    tok_linea = linea_actual;
    tok_col   = columna_actual;
    guardarAtomo(ATOMO_FIN, -1, 0, "$");

    imprimirTablas();
    imprimirAtomos();

    printf("\n======================================================\n");
    printf("RESULTADO DEL ANALISIS LEXICO\n");
    if (erroresLexicos > 0) {
        printf("Se encontraron %d error(es) lexico(s).\n", erroresLexicos);
        printf("NO se realizo el analisis sintactico.\n");
        printf("======================================================\n");
        return 1;
    }
    printf("Analisis lexico correcto (sin errores).\n");
    printf("======================================================\n");

    printf("\n--- ANALISIS SINTACTICO (descendente recursivo) ---\n");
    int errores = analizadorSintactico();

    printf("\n======================================================\n");
    printf("RESULTADO DEL ANALISIS SINTACTICO\n");
    if (errores == 0)
        printf("El archivo es sintacticamente CORRECTO.\n");
    else
        printf("Se encontraron %d error(es) sintactico(s). El archivo NO es sintacticamente correcto.\n", errores);
    printf("======================================================\n");
    return errores == 0 ? 0 : 2;
}
