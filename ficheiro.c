#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "ficheiro.h"
#include "cartas.h"


/* Garante que a pasta "saves/" existe. mkdir devolve -1 se já existir
   (errno=EEXIST), mas isso não é problema — o que conta é que ela exista. */
static void garantirPastaSaves(void) {
    mkdir("saves", 0755);
}


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

/* Constrói o caminho final do ficheiro de save:
   - Se o nome já tiver '/', respeita-o (o utilizador deu um caminho completo).
   - Caso contrário, antepõe "saves/" e, se faltar, acrescenta ".save". */
static void caminhoSave(const char *nome, char *dest, int tam) {
    if (strchr(nome, '/') != NULL) {
        snprintf(dest, tam, "%s", nome);
        return;
    }
    /* Acrescenta extensão .save se o utilizador não escreveu */
    const char *ponto = strrchr(nome, '.');
    if (ponto != NULL && strcmp(ponto, ".save") == 0)
        snprintf(dest, tam, "saves/%s", nome);
    else
        snprintf(dest, tam, "saves/%s.save", nome);
}

int gravarJogo(MotorPaciencia *m, const char *nome) {
    garantirPastaSaves();

    char caminho[160];
    caminhoSave(nome, caminho, sizeof(caminho));

    // Modo "w" (Write): Cria o ficheiro do zero. Se já existir um com este nome, apaga tudo e reescreve.
    FILE *f = fopen(caminho, "w");
    if (f == NULL) return 0; // Proteção: O SO pode recusar abrir por falta de permissões.

    fprintf(f, "%s\n", m->ficheiro);   /* 1.ª linha: ficheiro da paciência */

    for (int p = 0; p < m->num_pilhas; p++)
        gravarPilha(f, &m->mesa[p]);

    fclose(f); // Fecha o canal de comunicação para o ficheiro não ficar corrompido.
    return 1;
}

// Naipe a partir da letra (S/H/D/C -> 0/1/2/3). Devolve -1 se inválido.
static int naipeDe(char c) {
    if (c == 'S') return 0;
    if (c == 'H') return 1;
    if (c == 'D') return 2;
    if (c == 'C') return 3;
    return -1; /* letra desconhecida — token corrompido */
}

// Valor a partir do token "<valor><naipe>". Devolve 0 se não reconhecer
// (caso em que o token está mal formado).
static int valorDe(const char *tok) {
    if (tok[0] == 'A') return 1;
    if (tok[0] == 'J') return 11;
    if (tok[0] == 'Q') return 12;
    if (tok[0] == 'K') return 13;

    // Funcionalidade útil do C: O 'atoi' (ASCII to Integer) para automaticamente
    // assim que encontra um caractere não-numérico. Ou seja, se ler "10H",
    // ele apanha o "10" e ignora pacificamente o "H", sem dar erro!
    int v = atoi(tok);
    if (v >= 1 && v <= 13) return v;
    return 0; // fora do intervalo: token inválido
}

// Preenche a pilha p (já vazia) com as cartas descritas numa linha gravada.
// Os tokens vêm do fundo->topo, e empilhar() põe cada um no topo: a ordem fica certa.
// Tokens corrompidos (ex: "XX", "99Z") são silenciosamente saltados, com aviso
// no stderr — assim um save com 1 carta partida não estraga o resto da mesa
// nem provoca acessos fora dos limites no desenho.
static void carregarPilha(MotorPaciencia *m, int p, char *linha) {
    char tok[8];
    int pos = 0, lidos = 0;

    /* - `linha + pos` = Aritmética de apontadores. Faz o sscanf começar a ler mais à frente na frase.
       - `%7s` = Lê até 7 caracteres (evita buffer overflow no array tok).
       - `%n`  = Não lê texto, mas guarda na variável '&lidos' QUANTOS caracteres o sscanf acabou de consumir. */
    while (sscanf(linha + pos, "%7s%n", tok, &lidos) == 1) {
        int valor = valorDe(tok);
        int naipe = naipeDe(tok[strlen(tok) - 1]);

        if (valor == 0 || naipe == -1) {
            fprintf(stderr, "[!] Token '%s' invalido na pilha %d — ignorado.\n", tok, p);
        } else {
            Cartas c;
            c.valor = valor;
            c.naipe = naipe;
            empilhar(&m->mesa[p], c);
        }

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

/* Tenta abrir o ficheiro de save em vários sítios, por ordem:
   1) saves/<nome>.save   2) saves/<nome>   3) <nome>   4) <nome>.save
   Isto permite ao utilizador escrever apenas "partida1", e ainda assim
   conseguir abrir ficheiros que o professor deixe na raiz da pasta Motor. */
static FILE *abrirSave(const char *nome) {
    char tentativa[160];
    FILE *f;
    const char *ponto = strrchr(nome, '.');
    int jaTemExt = (ponto != NULL && strcmp(ponto, ".save") == 0);

    /* 1) saves/<nome>.save (ou saves/<nome> se já trouxe .save) */
    if (jaTemExt) snprintf(tentativa, sizeof(tentativa), "saves/%s", nome);
    else          snprintf(tentativa, sizeof(tentativa), "saves/%s.save", nome);
    f = fopen(tentativa, "r");
    if (f != NULL) return f;

    /* 2) saves/<nome> (sem extensão alterada) */
    snprintf(tentativa, sizeof(tentativa), "saves/%s", nome);
    f = fopen(tentativa, "r");
    if (f != NULL) return f;

    /* 3) <nome> (ficheiro tal-qual, ex: "golf-nearwin.save") */
    f = fopen(nome, "r");
    if (f != NULL) return f;

    /* 4) <nome>.save na pasta atual */
    if (!jaTemExt) {
        snprintf(tentativa, sizeof(tentativa), "%s.save", nome);
        f = fopen(tentativa, "r");
        if (f != NULL) return f;
    }
    return NULL;
}

/* 1 se o ficheiro existe e é legível; 0 caso contrário.
   Permite-nos validar a referência ao .paciencia antes de destruir o estado actual. */
static int ficheiroExiste(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

int carregarJogo(MotorPaciencia *m, const char *nome) {
    // Apenas lê. Tenta vários caminhos prováveis para o save.
    FILE *f = abrirSave(nome);
    if (f == NULL) return 0;

    char caminho[128], linha[512];  /* 512 cobre 1 baralho a 52 cartas (≈260 chars) com folga */

    // Passo 1: Lê o nome das regras originais na primeira linha
    if (lerCabecalho(f, caminho) == 0) { fclose(f); return 0; }

    // Passo 2: Pré-verifica que o .paciencia referenciado existe ANTES de
    // destruir o estado actual. Se não existir, abandona sem tocar no motor —
    // o utilizador continua no jogo que estava a jogar.
    if (ficheiroExiste(caminho) == 0) { fclose(f); return 0; }

    // Passo 3: Agora sim, limpa a memória atual para não sobrepor lixo
    inicializarMotor(m);

    // Passo 4: Pede ao parser.c para ler o ficheiro .paciencia original
    // e remontar as regras / limites da mesa antes de darmos as cartas!
    if (carregarPaciencia(m, caminho) == 0) { fclose(f); return 0; }

    // Passo 5: Lê as cartas salvas linha a linha e atira-as para as respetivas pilhas
    for (int p = 0; p < m->num_pilhas; p++)
        if (fgets(linha, sizeof(linha), f) != NULL) carregarPilha(m, p, linha);

    fclose(f);
    return 1;
}
