#ifndef MOTOR_H
#define MOTOR_H

#include "cartas.h" /* Aproveitamos a tua struct Cartas e Pilha intactas! */

#define MAX_TIPOS_PILHA 20
#define MAX_REGRAS 50
#define MAX_PILHAS_MESA 50
#define MAX_STR 50
#define MAX_UNDO 64   /* níveis de "voltar atrás" guardados */

/* Guarda a definição de um tipo de pilha (ex: "TAB", "=") */
typedef struct {
    char nome[MAX_STR];
    char flags[MAX_STR];
} TipoPilha;

/* Guarda uma regra de movimento (ex: "MOV", "STOCK", "DESCARTE", "*") */
typedef struct {
    char origem[MAX_STR];
    char destino[MAX_STR];
    char flags[MAX_STR];
    int  is_auto; /* 1 se for regra AUTO, 0 se for MOV */
} RegraMov;

/* Guarda um critério de vitória (ex: "WIN", "TAB", "0") */
typedef struct {
    char tipo_pilha[MAX_STR];
    int  quantidade_alvo;
} RegraWin;

/* O Novo Estado de Jogo Universal */
typedef struct {
    char      nome_jogo[MAX_STR];
    char      ficheiro[MAX_STR];   /* nome do ficheiro da paciência (p/ gravar) */
    int       num_baralhos;

    TipoPilha tipos[MAX_TIPOS_PILHA];
    int       num_tipos;

    Pilha     mesa[MAX_PILHAS_MESA];
    char      tipo_mesa[MAX_PILHAS_MESA][MAX_STR]; /* Guarda o tipo de cada pilha real */
    int       qtd_inicial[MAX_PILHAS_MESA];
    int       num_pilhas;

    RegraMov  regras_mov[MAX_REGRAS];
    int       num_regras_mov;

    RegraWin  regras_win[MAX_REGRAS];
    int       num_regras_win;

    /* Histórico para "voltar atrás": fotografias da mesa antes de cada jogada. */
    Pilha     historico[MAX_UNDO][MAX_PILHAS_MESA];
    int       hist_topo;           /* -1 = histórico vazio */

    /* Dica a destacar na mesa (roxo na origem, verde no destino); -1 = sem dica */
    int       dica_origem, dica_destino, dica_n;
} MotorPaciencia;

void inicializarMotor(MotorPaciencia *m);
int  carregarPaciencia(MotorPaciencia *m, const char *nome_ficheiro);
void distribuirCartasMesa(MotorPaciencia *m);            /* definida em setup.c  */
const char *flagsDoTipo(MotorPaciencia *m, const char *tipo); /* flags de um tipo */

#endif