/* ============================================================
 * atomos.h - Interfaz entre el analizador lexico (analizador.l),
 *            el modulo de atomos (atomos.c) y el sintactico (parser.c)
 * ============================================================ */
#ifndef ATOMOS_H
#define ATOMOS_H

#define MAX_ATOMOS   5000
#define MAX_LEXEMA   64
#define ATOMO_FIN    '$'      /* marcador de fin de archivo */

typedef struct {
    char atomo;                /* terminal de la gramatica (Tabla 1, Anexo A) */
    int  clase;                /* clase del componente lexico (-1 = fin)      */
    int  valor;                /* valor del token                             */
    int  linea;                /* linea donde inicia                          */
    int  columna;              /* columna donde inicia                        */
    char lexema[MAX_LEXEMA];   /* texto original (truncado), para errores     */
} Atomo;

extern Atomo listaAtomos[MAX_ATOMOS];
extern int   numAtomos;
extern int   erroresLexicos;   /* se define en analizador.l */

/* Lo llama analizador.l en cada token reconocido */
void agregarAtomo(char atomo, int clase, int valor, int linea, int columna, const char *lexema);

/* Lo llama main() al terminar de imprimir las tablas del Proyecto 1:
 * cierra la secuencia, la imprime y ejecuta el sintactico solo si no hubo errores lexicos */
int  finalizarAnalisis(int linea_fin, int columna_fin);

/* parser.c: devuelve el numero de errores sintacticos */
int  analizadorSintactico(void);

#endif
