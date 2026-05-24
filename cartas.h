#ifndef CARTAS_H
#define CARTAS_H

/* ── Tipos base ── */

// Uma carta do baralho.
typedef struct {
    int valor; /* 1=A, 2-10, 11=J, 12=Q, 13=K */
    int naipe; /* 0=Espadas, 1=Copas, 2=Ouros, 3=Paus */
} Cartas;

/* Nó de uma lista ligada de cartas: cada carta aponta para a que está
   imediatamente por baixo dela na pilha. */
typedef struct No {
    Cartas      carta;
    struct No  *prox;   /* carta de baixo; NULL no fundo da pilha */
} No;

/* Pilha (stack) DINÂMICA: guarda apenas o apontador para a carta do topo.
   Substitui o antigo array fixo — cresce e encolhe com malloc/free. */
typedef struct {
    No *topo;           /* NULL = pilha vazia */
} Pilha;

/* ── Operações de pilha (stack) ── */
void   empilhar   (Pilha *p, Cartas c); /* põe c no topo (push)                  */
Cartas desempilhar(Pilha *p);           /* tira e devolve o topo (pop); não vazia */
int    pilhaVazia (Pilha *p);           /* 1 se a pilha está vazia                */
int    alturaPilha(Pilha *p);           /* número de cartas na pilha             */
Cartas cartaTopo  (Pilha *p);           /* carta do topo (assume não vazia)       */
Cartas cartaNivel (Pilha *p, int i);    /* i-ésima carta a contar do fundo (0=fundo) */
No    *copiarLista(No *topo);           /* cópia profunda da lista (para o undo)  */
void   libertarLista(No *topo);         /* liberta todos os nós da lista          */

/* ── Acessores de texto (sem variáveis globais) ── */
const char *simboloNaipe(int naipe); /* símbolo UTF-8 do naipe   */
const char *letraValor (int valor);  /* "A".."K" do valor        */
char        naipeLetra (int naipe);  /* 'S','H','D','C' do naipe */

#endif
