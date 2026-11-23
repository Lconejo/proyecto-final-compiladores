/* atomos.c - Secuencia de atomos y control del analisis */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atomos.h"

Atomo listaAtomos[MAX_ATOMOS];
int   numAtomos = 0;

void agregarAtomo(char atomo, int clase, int valor, int linea, int columna, const char *lexema) {
    if (numAtomos >= MAX_ATOMOS) {
        fprintf(stderr, "Demasiados atomos (maximo %d).\n", MAX_ATOMOS);
        exit(1);
    }
    Atomo *a = &listaAtomos[numAtomos++];
    a->atomo   = atomo;
    a->clase   = clase;
    a->valor   = valor;
    a->linea   = linea;
    a->columna = columna;
    strncpy(a->lexema, lexema, MAX_LEXEMA - 1);
    a->lexema[MAX_LEXEMA - 1] = '\0';
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

int finalizarAnalisis(int linea_fin, int columna_fin) {
    agregarAtomo(ATOMO_FIN, -1, 0, linea_fin, columna_fin, "$");
    imprimirAtomos();

    printf("\n======================================================\n");
    printf("RESULTADO DEL ANALISIS LEXICO\n");
    if (erroresLexicos > 0) {
        printf("Se encontraron %d error(es) lexico(s).\n", erroresLexicos);
        printf("NO se realizo el analisis sintactico :( \n");
        printf("======================================================\n");
        return 1;
    }
    printf("Analisis lexico correcto y sin errores :D\n");
    int errores = analizadorSintactico();

    printf("\n======================================================\n");
    printf("RESULTADO DEL ANALISIS SINTACTICO\n");
    if (errores == 0)
        printf("El archivo es sintacticamente correcto :D\n");
    else
        printf("Se encontraron %d error(es) sintactico(s). El archivo no es sintacticamente correcto :(\n", errores);
    printf("======================================================\n");
    return errores == 0 ? 0 : 2;
}
