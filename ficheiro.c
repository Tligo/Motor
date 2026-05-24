#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheiro.h"
#include "cartas.h"


// Escreve os nós do fundo para o topo (recursivo: primeiro os de baixo).
static void gravarNos(FILE *f, No *n) {
    if (n == NULL) return; /* Caso base: Chegou ao fim da pilha (fundo) */
    
    // Truque clássico de recursividade: Como a lista ligada só desce (topo -> fundo),
    // e o enunciado pede para imprimir do fundo para o topo, fazemos a chamada recursiva
    // ANTES do fprintf. Assim, as cartas vão ficando "em espera" na memória e só são 
    // impressas quando o ciclo volta para trás (da base até ao topo).
    gravarNos(f, n->prox);   /* os que estão por baixo saem primeiro (à esquerda) */
    
    fprintf(f, "%s%c ", letraValor(n->carta.valor), naipeLetra(n->carta.naipe));
}

// Escreve uma pilha numa linha (cartas do fundo->topo) e termina com '\n'.
static void gravarPilha(FILE *f, Pilha *p) {
    gravarNos(f, p->topo);
    // Carriage return (\n) vital: O enunciado dita que cada pilha é uma linha isolada.
    fprintf(f, "\n"); 
}

int gravarJogo(MotorPaciencia *m, const char *nome) {
    // Modo "w" (Write): Cria o ficheiro do zero. Se já existir um com este nome, apaga tudo e reescreve.
    FILE *f = fopen(nome, "w");
    if (f == NULL) return 0; // Proteção: O SO pode recusar abrir por falta de permissões.
    
    fprintf(f, "%s\n", m->ficheiro);   /* 1.ª linha: ficheiro da paciência */
    
    for (int p = 0; p < m->num_pilhas; p++)
        gravarPilha(f, &m->mesa[p]);
        
    fclose(f); // Fecha o canal de comunicação para o ficheiro não ficar corrompido.
    return 1;
}

// Naipe a partir da letra (S/H/D/C -> 0/1/2/3).
static int naipeDe(char c) {
    if (c == 'S') return 0;
    if (c == 'H') return 1;
    if (c == 'D') return 2;
    return 3; /* 'C' */
}

// Valor a partir do token "<valor><naipe>" (atoi ignora a letra do naipe).
static int valorDe(const char *tok) {
    if (tok[0] == 'A') return 1;
    if (tok[0] == 'J') return 11;
    if (tok[0] == 'Q') return 12;
    if (tok[0] == 'K') return 13;
    
    // Funcionalidade útil do C: O 'atoi' (ASCII to Integer) para automaticamente
    // assim que encontra um caractere não-numérico. Ou seja, se ler "10H",
    // ele apanha o "10" e ignora pacificamente o "H", sem dar erro!
    return atoi(tok);
}

// Preenche a pilha p (já vazia) com as cartas descritas numa linha gravada.
// Os tokens vêm do fundo->topo, e empilhar() põe cada um no topo: a ordem fica certa.
static void carregarPilha(MotorPaciencia *m, int p, char *linha) {
    char tok[8];
    int pos = 0, lidos = 0;
    
    /* - `linha + pos` = Aritmética de apontadores. Faz o sscanf começar a ler mais à frente na frase.
       - `%7s` = Lê até 7 caracteres (evita buffer overflow no array tok).
       - `%n`  = Não lê texto, mas guarda na variável '&lidos' QUANTOS caracteres o sscanf acabou de consumir. */
    while (sscanf(linha + pos, "%7s%n", tok, &lidos) == 1) {
        Cartas c;
        c.valor = valorDe(tok);
        // strlen(tok) - 1: Vai buscar sempre a última letra da palavra (que é o naipe).
        c.naipe = naipeDe(tok[strlen(tok) - 1]);
        
        empilhar(&m->mesa[p], c);
        
        // Atualiza a posição de leitura, empurrando o apontador para a frente na frase.
        pos = pos + lidos;
    }
}

// Lê a 1.ª linha (ficheiro) e monta o caminho "paciencias/<ficheiro>".
static int lerCabecalho(FILE *f, char *caminho) {
    char linha[128];
    if (fgets(linha, sizeof(linha), f) == NULL) return 0;
    
    int len = strlen(linha);
    // Substitui o 'ENTER' que o fgets apanhou pelo terminador de string nulo (\0).
    // Se não fizéssemos isto, a concatenação ia quebrar a string a meio.
    if (len > 0 && linha[len - 1] == '\n') linha[len - 1] = '\0'; // tira o '\n'
    
    // Junta as strings ("paciencias/" + "nome_do_jogo.paciencia")
    strcpy(caminho, "paciencias/");
    strcat(caminho, linha);
    return 1;
}

int carregarJogo(MotorPaciencia *m, const char *nome) {
    // Apenas lê. Falha imediatamente (f == NULL) se o ficheiro não existir.
    FILE *f = fopen(nome, "r");
    if (f == NULL) return 0;
    
    char caminho[128], linha[256];
    
    // Passo 1: Lê o nome das regras originais na primeira linha
    if (lerCabecalho(f, caminho) == 0) { fclose(f); return 0; }
    
    // Passo 2: Limpa a memória atual para não sobrepor lixo
    inicializarMotor(m);
    
    // Passo 3: Pede ao teu 'parser.c' para ler o ficheiro .paciencia original
    // e remontar as regras / limites da mesa antes de darmos as cartas!
    if (carregarPaciencia(m, caminho) == 0) { fclose(f); return 0; }
    
    // Passo 4: Lê as cartas salvas linha a linha e atira-as para as respetivas pilhas
    for (int p = 0; p < m->num_pilhas; p++)
        if (fgets(linha, sizeof(linha), f) != NULL) carregarPilha(m, p, linha);
        
    fclose(f);
    return 1;
}