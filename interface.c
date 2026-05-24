#include <stdio.h>
#include <string.h>
#include "interface.h"

/* ──────────────────────────────────────────────────────────────────
 * Desenho da mesa em caixas (┌─────┐), em colunas, com cores e símbolos,
 * no mesmo estilo do Golf/Simple Simon. Reutiliza letraValor/simboloNaipe e
 * as operações de pilha (cartas.c) e flagsDoTipo (parser.c). Uma coluna por
 * pilha. Pilhas totalmente tapadas (ex.: STOCK) mostram-se como um baralho
 * compacto com a contagem, para não ficarem dezenas de caixas de altura.
 * ────────────────────────────────────────────────────────────────── */

/* 1 se a pilha é totalmente tapada (sem '=' nem '^'): mostra-se como baralho. */
static int pilhaBaralho(MotorPaciencia *m, int p) {
    const char *flags = flagsDoTipo(m, m->tipo_mesa[p]);
    // O professor pode perguntar sobre o strchr: Ele procura um caractere numa string.
    // Se devolver NULL, significa que a flag de visibilidade não existe na regra daquela pilha.
    return strchr(flags, '=') == NULL && strchr(flags, '^') == NULL;
}

/* 1 se a carta no nível i da pilha p está visível ('='=todas, '^'=só o topo). */
static int cartaVisivel(MotorPaciencia *m, int p, int i) {
    const char *flags = flagsDoTipo(m, m->tipo_mesa[p]);
    if (strchr(flags, '=')) return 1;
    // Lógica vital: Se a flag for '^', a carta só fica visível se o seu índice (i) for exatamente o topo da pilha.
    if (strchr(flags, '^')) return i == alturaPilha(&m->mesa[p]) - 1;
    return 0;
}

/* Nº de níveis a desenhar para a pilha p (baralho compacto conta como 1). */
static int niveisPilha(MotorPaciencia *m, int p) {
    // Truque de Interface Gráfica: Se a pilha for um Stock (baralho virado para baixo),
    // fingimos que ela só tem tamanho "1" para o terminal não imprimir 52 caixas vazias para baixo.
    if (pilhaBaralho(m, p)) return pilhaVazia(&m->mesa[p]) ? 0 : 1;
    return alturaPilha(&m->mesa[p]);
}

/* Altura da pilha mais alta (nº de níveis de cartas a desenhar). */
static int maxAltura(MotorPaciencia *m) {
    int max = 0;
    for (int p = 0; p < m->num_pilhas; p++)
        if (niveisPilha(m, p) > max) max = niveisPilha(m, p);
    return max;
}

/* 1 se há célula a desenhar para a pilha p no nível lin. */
static int temCelula(MotorPaciencia *m, int p, int lin) {
    if (pilhaBaralho(m, p)) return lin == 0 && !pilhaVazia(&m->mesa[p]);
    return lin < alturaPilha(&m->mesa[p]);
}

/* Destaque da dica: 1=origem (roxo), 2=destino (verde), 0=nenhum. */
static int destaqueCarta(MotorPaciencia *m, int p, int lin) {
    int topo = alturaPilha(&m->mesa[p]) - 1;
    if (pilhaBaralho(m, p)) topo = 0; /* o baralho é uma só caixa no nível 0 */
    
    // Matemática de índices: Subtrai a quantidade de cartas da dica ao topo para descobrir 
    // a que profundidade (linha) começa a cor roxa.
    if (p == m->dica_origem && lin >= topo - m->dica_n + 1) return 1;
    if (p == m->dica_destino && lin == topo)                return 2;
    return 0;
}

/* Liga a cor do terminal: destaque tem prioridade sobre a cor do naipe. */
static void ligarCor(Cartas c, int destaque, int visivel) {
    if      (destaque == 1)                             printf(COR_ROXA);
    else if (destaque == 2)                             printf(COR_VERDE);
    // Só imprime vermelho se não for uma dica E a carta estiver de facto visível E for de Ouros/Copas
    else if (visivel && (c.naipe == 1 || c.naipe == 2)) printf(COR_VERMELHA);
}

/* Conteúdo do meio da caixa (5 chars): valor+naipe, baralho (contagem) ou tapada. */
static void meioCelula(MotorPaciencia *m, int p, int lin) {
    // %2d força o número a ocupar sempre 2 espaços (ex: " 5" ou "52"), mantendo a parede direita da carta alinhada.
    if (pilhaBaralho(m, p))       { printf("│ %2d  │", alturaPilha(&m->mesa[p])); return; }
    
    if (!cartaVisivel(m, p, lin)) { printf("│  ?  │"); return; }
    Cartas c = cartaNivel(&m->mesa[p], lin);
    
    // %-2s empurra o texto para a esquerda (ex: o "10" ocupa os dois espaços, mas o "A" deixa um espaço em branco à direita).
    printf("│ %-2s%s │", letraValor(c.valor), simboloNaipe(c.naipe));
}

/* Desenha uma das três partes (1=topo, 2=meio, 3=fundo) de uma célula. */
static void imprimirCelula(MotorPaciencia *m, int p, int lin, int parte) {
    int    dest = destaqueCarta(m, p, lin);
    int    vis  = cartaVisivel(m, p, lin);
    Cartas c    = {0, 0};
    if (vis) c = cartaNivel(&m->mesa[p], lin);
    
    ligarCor(c, dest, vis);
    if      (parte == 1) printf("┌─────┐");
    else if (parte == 3) printf("└─────┘");
    else                 meioCelula(m, p, lin);
    
    // Prevenção de "Bleeding" visual: Obrigatório desligar a cor (COR_RESET) no fim de cada carta, 
    // senão o terminal ficaria todo roxo ou verde para sempre.
    printf(COR_RESET);
}

/* Caixa verde vazia: marca o destino da dica quando a pilha está vazia. */
static void marcadorVazio(int parte) {
    printf(COR_VERDE);
    if      (parte == 1) printf("┌─────┐");
    else if (parte == 3) printf("└─────┘");
    else                 printf("│     │");
    printf(COR_RESET);
}

/* 1 se aqui se deve mostrar o marcador de destino vazio da dica. */
static int destinoVazio(MotorPaciencia *m, int p, int lin) {
    return lin == 0 && p == m->dica_destino && pilhaVazia(&m->mesa[p]);
}

/* Imprime a parte (1/2/3) de todas as pilhas no nível lin. */
static void imprimirLinha(MotorPaciencia *m, int lin, int parte) {
    for (int p = 0; p < m->num_pilhas; p++) {
        if      (temCelula(m, p, lin))    imprimirCelula(m, p, lin, parte);
        else if (destinoVazio(m, p, lin)) marcadorVazio(parte);
        else                              printf("       "); /* Enchimento para não quebrar a grelha */
        printf(" ");
    }
    printf("\n");
}

/* Cabeçalho: número de cada pilha (é o que se usa no comando 'move'). */
static void imprimirCabecalho(MotorPaciencia *m) {
    // %02d imprime sempre com zero à esquerda se for menor que 10 (ex: 00, 01, 09, 10).
    for (int p = 0; p < m->num_pilhas; p++) printf("  %02d    ", p);
    printf("\n\n");
}

/* Função principal: desenha a mesa completa. */
void mostrarMesa(MotorPaciencia *m) {
    printf("\n===== %s =====\n\n", m->nome_jogo);
    imprimirCabecalho(m);
    
    // Arquitetura de impressão horizontal: Como o terminal não consegue subir para desenhar as cartas 
    // coluna a coluna, o C tem de desenhar a mesa em "fatias" horizontais.
    int linhas = maxAltura(m);
    for (int lin = 0; lin < linhas; lin++) {
        imprimirLinha(m, lin, 1); // 1º Passa por todas as pilhas e desenha os tetos
        imprimirLinha(m, lin, 2); // 2º Passa por todas as pilhas e desenha os valores
        imprimirLinha(m, lin, 3); // 3º Passa por todas as pilhas e desenha os fundos
    }
    printf("\n");
}