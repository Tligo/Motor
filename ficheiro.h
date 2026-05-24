#ifndef FICHEIRO_H
#define FICHEIRO_H

#include "motor.h"

/* Grava o estado atual no formato do enunciado:
 *   linha 1: nome do ficheiro da paciência;
 *   uma linha por pilha (cartas do fundo->topo, "<valor><naipe>" separadas por
 *   espaços; pilha vazia => linha vazia).
 * Devolve 1 em sucesso, 0 em erro. */
int gravarJogo(MotorPaciencia *m, const char *nome);

/* Carrega um estado gravado: lê a paciência indicada na 1.ª linha (da pasta
 * "paciencias") e repõe as cartas de cada pilha. Devolve 1 em sucesso, 0 em erro. */
int carregarJogo(MotorPaciencia *m, const char *nome);

#endif
