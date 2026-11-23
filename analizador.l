/* ============================================================
 * analizador.l - Analizador lexico (Flex) del lenguaje X
 * Proyecto 2, Parte 2: integrado con el analizador sintactico
 *
 * Compilacion:
 *     flex analizador.l
 *     gcc lex.yy.c atomos.c parser.c -o analizador
 * Uso:
 *     analizador archivo_fuente.txt
 * ============================================================ */
%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atomos.h"

/* Antes de cada regla: registra linea/columna donde INICIA el lexema */
#define YY_USER_ACTION actualizarPosicion(yytext, yyleng);
%}

%option noyywrap

/* Expresiones regulares */

/* Clase 0: Operadores aritmeticos */
op_arit     (\+|-|\/|\*|%)

/* Clase 3: Constantes numericas base 10 con o sin signo */
digito        [0-9]
dig_signo     \([+-]{digito}+\)
ctes_numerica {digito}+|{dig_signo}

/* Clase 4: Palabras reservadas */
pal_res       (assinado|caso|enquanto|fazer|flutuador|inteiro|para|quebrar|retorno|se|trocar)

/* Clase 5: Identificadores */
letra         [a-zA-Z]
identificador _{letra}({letra}|{digito}|_)*

/* Clase 6: Simbolos especiales */
simb_esp      (\(|\)|\{|\}|;|,|\[|\]|:|#)

/* Clase 8: Cadenas encerradas entre comillas */
cadena        \"([^"\\]|\\.)*\"

%%

 /* Control de espacios (las lineas/columnas las lleva YY_USER_ACTION) */
\n          { }
[ \t\r]+    { }

 /* Clase 0: Operadores aritmeticos (atomo = el mismo simbolo) */
{op_arit}   { emitirToken(0, yytext[0], yytext[0], yytext); }

 /* Clase 1: Operadores logicos  (&& -> h, || -> j, ! -> !) */
"&&"        { emitirToken(1, 0, 'h', yytext); }
"||"        { emitirToken(1, 1, 'j', yytext); }
"!"         { emitirToken(1, 2, '!', yytext); }

 /* Clase 2: Operadores relacionales (== e, != d, > >, < <, >= m, <= w) */
"=="        { emitirToken(2, 0, 'e', yytext); }
"!="        { emitirToken(2, 1, 'd', yytext); }
">"         { emitirToken(2, 2, '>', yytext); }
"<"         { emitirToken(2, 3, '<', yytext); }
">="        { emitirToken(2, 4, 'm', yytext); }
"<="        { emitirToken(2, 5, 'w', yytext); }

 /* Clase 3: Constantes numericas (atomo n) */
{ctes_numerica} {
    int valorCte = (yytext[0] == '(') ? atoi(&yytext[1]) : atoi(yytext);
    emitirToken(3, valorCte, 'n', yytext);
}

 /* Clase 4: Palabras reservadas */
{pal_res} {
    int  valor;
    char atomo;
    if      (strcmp(yytext, "assinado")  == 0) { valor = 0;  atomo = 'g'; }
    else if (strcmp(yytext, "caso")      == 0) { valor = 1;  atomo = 'c'; }
    else if (strcmp(yytext, "enquanto")  == 0) { valor = 2;  atomo = 'q'; }
    else if (strcmp(yytext, "fazer")     == 0) { valor = 3;  atomo = 'z'; }
    else if (strcmp(yytext, "flutuador") == 0) { valor = 4;  atomo = 'f'; }
    else if (strcmp(yytext, "inteiro")   == 0) { valor = 5;  atomo = 'i'; }
    else if (strcmp(yytext, "para")      == 0) { valor = 6;  atomo = 'p'; }
    else if (strcmp(yytext, "quebrar")   == 0) { valor = 7;  atomo = 'u'; }
    else if (strcmp(yytext, "retorno")   == 0) { valor = 8;  atomo = 't'; }
    else if (strcmp(yytext, "se")        == 0) { valor = 9;  atomo = 's'; }
    else                                       { valor = 10; atomo = 'b'; }  /* trocar */
    emitirToken(4, valor, atomo, yytext);
}

 /* Clase 5: Identificadores (atomo a) */
{identificador} {
    int indice = guardarIden(yytext);
    emitirToken(5, indice, 'a', yytext);
}

 /* Clase 6: Simbolos especiales (atomo = el mismo simbolo) */
{simb_esp}  { emitirToken(6, yytext[0], yytext[0], yytext); }

 /* Clase 7: Operadores de asignacion (= =, += x, -= y, *= k, /= r, %= o) */
"+="        { emitirToken(7, 1, 'x', yytext); }
"-="        { emitirToken(7, 2, 'y', yytext); }
"*="        { emitirToken(7, 3, 'k', yytext); }
"/="        { emitirToken(7, 4, 'r', yytext); }
"%="        { emitirToken(7, 5, 'o', yytext); }
"="         { emitirToken(7, 0, '=', yytext); }

 /* Clase 8: Constantes cadena (atomo v). Los saltos de linea internos
    ya los contabiliza YY_USER_ACTION. */
{cadena} {
    int pos = guardarLiteral(yytext);
    emitirToken(8, pos, 'v', yytext);
}

 /* Clase 9: Operadores sobre cadenas (& -> &, like -> l) */
"&"         { emitirToken(9, 0, '&', yytext); }
"like"      { emitirToken(9, 1, 'l', yytext); }

 /* Errores lexicos: se reportan y el analisis continua */
.           { reportarErrorLexico(yytext); }

%%

int main(int argc, char *argv[]) {
    imprimirEncabezado();

    if (argc < 2) {
        printf("Falta el archivo de entrada.\n");
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        printf("Error al abrir el archivo.\n");
        return 1;
    }

    printf("\n--- TOKENS GENERADOS (Clase, Valor) ---\n");
    yylex();
    fclose(yyin);

    return finalizarAnalisis();
}
