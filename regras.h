#ifndef REGRAS_H
#define REGRAS_H

#include "motor.h"

/* ──────────────────────────────────────────────────────────────────
 * Módulo de regras de movimento (LÓGICA — sem qualquer input/output).
 * Interpreta as flags dos comandos MOV/AUTO da DSL e decide se uma
 * jogada é legal. Toda a lógica daqui é testável com CUnit.
 * ────────────────────────────────────────────────────────────────── */

/* Cor de uma carta: 0 = preto (Espadas, Paus), 1 = vermelho (Copas, Ouros). */
int corCarta(Cartas c);

/* Verifica se é legal mover as 'n' cartas do topo da pilha 'origem' para
 * a pilha 'destino', segundo as regras MOV da paciência carregada.
 * Regra aplicável = DISJUNÇÃO dos MOV cujos tipos correspondem; dentro de
 * cada MOV, as flags são uma CONJUNÇÃO. Devolve 1 se legal, 0 caso contrário. */
int validarMovimento(MotorPaciencia *m, int origem, int destino, int n);

/* Valida e, se legal, executa o movimento: retira as n cartas do topo da
 * pilha 'origem' e coloca-as no topo da 'destino', preservando a ordem.
 * Devolve 1 se o movimento foi executado, 0 se era inválido (estado intacto). */
int executarMovimento(MotorPaciencia *m, int origem, int destino, int n);

/* Como executarMovimento mas escolhe sozinho o nº de cartas (sequência movível).
 * Devolve 1 se moveu, 0 se nenhuma quantidade era válida. */
int executarMovimentoAuto(MotorPaciencia *m, int origem, int destino);

/* Aplica em cadeia todas as regras AUTO que se verifiquem, até estabilizar.
 * Deve ser chamada após cada jogada do utilizador. */
void aplicarAutomaticos(MotorPaciencia *m);

/* Dica: procura uma jogada válida (reutiliza validarMovimento). Se encontrar,
 * preenche *origem, *destino e *n e devolve 1; devolve 0 se não houver. */
int procurarDica(MotorPaciencia *m, int *origem, int *destino, int *n);

/* Devolve 1 se a conjunção das regras WIN se verifica (vitória), 0 caso
 * contrário (e 0 se a paciência não definir qualquer regra WIN). */
int verificarVitoria(MotorPaciencia *m);

/* Devolve 1 se o jogo está em derrota: NÃO ganhou e não existe nenhuma
 * jogada válida (procurarDica falha). Em paciências com STOCK→DESCARTE *
 * isto significa, na prática, "stock vazio e nada combina no descarte". */
int verificarDerrota(MotorPaciencia *m);

/* Histórico para voltar atrás (undo). */
void guardarHistorico(MotorPaciencia *m);   /* fotografa a mesa antes da jogada */
void descartarHistorico(MotorPaciencia *m); /* anula a última fotografia        */
int  desfazerJogada(MotorPaciencia *m);     /* restaura a mesa anterior (1/0)   */

#endif
