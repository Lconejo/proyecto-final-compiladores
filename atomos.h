/* ============================================================
 * atomos.h - Interfaz comun entre el analizador lexico (Flex),
 *            el modulo de reportes (atomos.c) y el analizador
 *            sintactico descendente recursivo (parser.c)
 * ============================================================ */
#ifndef ATOMOS_H
#define ATOMOS_H

#define MAX_ATOMOS   5000
#define MAX_LEXEMA   64
#define ATOMO_FIN    '$'      /* marcador de fin de archivo (el simbolo de fin de las tablas) */

/* Atomo que el analizador lexico entrega al sintactico */
typedef struct {
    char atomo;                /* simbolo terminal de la gramatica (Tabla 1, Anexo A) */
    int  clase;                /* clase del componente lexico (0-9), -1 para fin     */
    int  valor;                /* valor del token                                     */
    int  linea;                /* linea donde inicia en el fuente                     */
    int  columna;              /* columna donde inicia en el fuente                   */
    char lexema[MAX_LEXEMA];   /* texto original (truncado) para mensajes de error    */
} Atomo;

extern Atomo listaAtomos[MAX_ATOMOS];
extern int   numAtomos;
extern int   erroresLexicos;

/* Posicion del token actual (la actualiza actualizarPosicion) */
extern int tok_linea, tok_col;

/* ---- Servicios para el analizador lexico (llamados desde analizador.l) ---- */
void actualizarPosicion(const char *texto, int longitud);
void emitirToken(int clase, int valor, char atomo, const char *lexema);
void reportarErrorLexico(const char *lexema);
int  guardarIden(const char *nombre);
int  guardarLiteral(const char *cadena);

/* ---- Reportes y control general ---- */
void imprimirEncabezado(void);
int  finalizarAnalisis(void);   /* tablas, atomos, y (si procede) analisis sintactico */

/* ---- Analizador sintactico (parser.c) ---- */
int  analizadorSintactico(void); /* devuelve el numero de errores sintacticos */

#endif
