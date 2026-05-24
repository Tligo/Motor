#include <stdlib.h>
#include "cartas.h"


// Põe a carta c no topo da pilha (cria um nó novo e liga-o ao antigo topo).
void empilhar(Pilha *p, Cartas c) {
    // Alocação Dinâmica: Pede memória ao SO em tempo de execução para criar a nova "caixa" (Nó).
    No *n = malloc(sizeof(No));
    n->carta = c;
    // O novo nó "agarra" o topo antigo, empurrando a pilha para baixo.
    n->prox  = p->topo;
    // O topo oficial da pilha passa a apontar para este novo nó.
    p->topo  = n;
}

// Tira a carta do topo e devolve-a, libertando o nó. Assume pilha não vazia.
Cartas desempilhar(Pilha *p) {
    No *n      = p->topo;
    Cartas c   = n->carta;
    // A pilha encolhe: o topo passa a ser o segundo elemento da lista.
    p->topo    = n->prox;
    // Liberta a memória do nó removido para evitar Memory Leaks (fugas de memória).
    free(n);
    return c;
}

// Devolve 1 se a pilha estiver vazia.
int pilhaVazia(Pilha *p) {
    return p->topo == NULL;
}

// Conta as cartas da pilha, percorrendo a lista do topo ao fundo.
int alturaPilha(Pilha *p) {
    int n = 0;
    // Navegação clássica em Listas Ligadas: Começa no topo e avança (atual = atual->prox) até bater no fundo (NULL).
    for (No *atual = p->topo; atual != NULL; atual = atual->prox)
        n++;
    return n;
}

// Devolve a carta do topo. Assume pilha não vazia.
Cartas cartaTopo(Pilha *p) {
    return p->topo->carta;
}

// Devolve a i-ésima carta a contar do fundo (0 = fundo). Assume i válido.
Cartas cartaNivel(Pilha *p, int i) {
    // Como a Lista Ligada só desce (do topo para o fundo), e tu queres contar de baixo para cima,
    // é obrigatório calcular a altura total primeiro para saber quantos "saltos" dar a partir do topo.
    int passos = alturaPilha(p) - 1 - i; /* nº de saltos do topo até ao nível i */
    No *atual  = p->topo;
    while (passos > 0 && atual != NULL) {
        atual = atual->prox;
        passos--;
    }
    return atual->carta;
}

// Cópia profunda de uma lista (nós novos), usada para guardar estados (undo).
No *copiarLista(No *topo) {
    if (topo == NULL) return NULL; // Caso base da recursividade
    
    // Não podemos copiar apenas os apontadores, temos de alocar memória
    // para um nó totalmente novo para que o Histórico não altere a mesa atual.
    No *novo   = malloc(sizeof(No));
    novo->carta = topo->carta;
    novo->prox  = copiarLista(topo->prox); // Chamada recursiva constrói o resto da lista magicamente
    return novo;
}

// Liberta todos os nós de uma lista.
void libertarLista(No *topo) {
    while (topo != NULL) {
        // Temos de guardar o próximo nó ANTES de apagar o atual.
        // Se fizéssemos free(topo) primeiro, ao tentar ler topo->prox o programa estoirava (Use-After-Free).
        No *seguinte = topo->prox;
        free(topo);
        topo = seguinte;
    }
}


// Devolve o símbolo UTF-8 do naipe (0=Espadas,1=Copas,2=Ouros,3=Paus).
const char *simboloNaipe(int naipe) {
    // Truque mestre: O 'static' dentro da função cria o array uma única vez na memória.
    // Assim, contornas a regra proibitiva das "Variáveis Globais", mantendo a eficiência!
    static const char *simbolos[] = {
        "\xE2\x99\xA0",  /* ♠ Espadas */
        "\xE2\x99\xA5",  /* ♥ Copas   */
        "\xE2\x99\xA6",  /* ♦ Ouros   */
        "\xE2\x99\xA3"   /* ♣ Paus    */
    };
    return simbolos[naipe];
}

// Devolve a representação textual do valor (1=A, 2-10, 11=J, 12=Q, 13=K).
const char *letraValor(int valor) {
    static const char *letras[] = {
        "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
    };
    return letras[valor];
}

// Devolve a letra do naipe no formato gravado: 0->S 1->H 2->D 3->C.
char naipeLetra(int naipe) {
    static const char letras[] = {'S', 'H', 'D', 'C'};
    return letras[naipe];
}