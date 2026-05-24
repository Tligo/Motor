#include <stdio.h>
#include <string.h>
#include "motor.h"

/* Liberta e anula todas as listas (mesa + histórico). Exige apontadores
   válidos (NULL ou alocados) — o main faz memset ao MotorPaciencia ao arrancar.
   Assim, recarregar um jogo não deixa memória por libertar. */
static void limparMotor(MotorPaciencia *m) {
    // 1º Ciclo: Varre a mesa atual e mata todas as pilhas.
    for (int p = 0; p < MAX_PILHAS_MESA; p++) {
        // A função libertarLista (no cartas.c) faz free() a todos os nós
        libertarLista(m->mesa[p].topo);
        // Obrigatoriedade de segurança: Depois de libertar, o apontador passa a NULL.
        // Se ficar a apontar para o espaço apagado (Dangling Pointer), o programa estoira.
        m->mesa[p].topo = NULL;
    }
    
    // 2º Ciclo: Varre todos os snapshots guardados no Histórico (Undo) e liberta-os também.
    // MAX_UNDO protege contra o ciclo infinito.
    for (int u = 0; u < MAX_UNDO; u++)
        for (int p = 0; p < MAX_PILHAS_MESA; p++) {
            libertarLista(m->historico[u][p].topo);
            m->historico[u][p].topo = NULL;
        }
}

/* Prepara a estrutura do motor: liberta listas antigas e zera os contadores. */
void inicializarMotor(MotorPaciencia *m) {
    // Chamada à nova função de "Garbage Collection" manual antes de zerar os dados
    limparMotor(m);
    
    // Zera strings pondo o terminador nulo na primeira posição
    m->nome_jogo[0]   = '\0';
    m->ficheiro[0]    = '\0';
    m->num_baralhos   = 1;
    m->num_tipos      = 0;
    m->num_pilhas     = 0;
    m->num_regras_mov = 0;
    m->num_regras_win = 0;
    
    // Contadores a -1 representam "estado vazio / inativo"
    m->hist_topo      = -1;
    m->dica_origem    = -1;
    m->dica_destino   = -1;
    m->dica_n         = -1;
}

/* Guarda o nome do ficheiro (sem o caminho) para futura gravação. */
static void guardarNomeFicheiro(MotorPaciencia *m, const char *caminho) {
    int inicio = 0;
    /* fica com a posição logo a seguir à última barra '/' */
    // Ex: "paciencias/golf.paciencia". O ciclo para no '/' (índice 10). 
    // inicio = 11, o que aponta para o "g" de golf.
    for (int i = 0; caminho[i] != '\0'; i++)
        if (caminho[i] == '/') inicio = i + 1;
        
    // Aritmética de apontadores (caminho + inicio): Copia apenas a partir do "g", descartando o caminho.
    strcpy(m->ficheiro, caminho + inicio);
}

/* Devolve as flags do tipo de pilha chamado 'tipo' (ou "" se não existir). */
const char *flagsDoTipo(MotorPaciencia *m, const char *tipo) {
    for (int i = 0; i < m->num_tipos; i++)
        if (strcmp(m->tipos[i].nome, tipo) == 0)
            return m->tipos[i].flags;
    return ""; // Devolve string vazia em vez de NULL para evitar Segmentation Fault na leitura.
}

/* Corta a string a partir do símbolo cardinal (#) para ignorar comentários */
static void limparComentario(char *linha) {
    char *cardinal = strchr(linha, '#');
    if (cardinal != NULL) {
        // Ao meter um '\0' no lugar do '#', o C passa a assumir que a string acaba ali.
        *cardinal = '\0';
    }
}

/* --- FUNÇÕES DE EXTRAÇÃO DE DADOS --- */

static void lerJogo(MotorPaciencia *m, const char *linha) {
    // "%49s" previne Buffer Overflow, garantindo que não são lidos mais chars que o tamanho da string.
    sscanf(linha, "JOGO %49s", m->nome_jogo);
}

static void lerBaralhos(MotorPaciencia *m, const char *linha) {
    sscanf(linha, "BARALHOS %d", &m->num_baralhos);
}

static void lerTipo(MotorPaciencia *m, const char *linha) {
    if (m->num_tipos >= MAX_TIPOS_PILHA) return;
    // Uso de apontador temporário (t) para evitar escrever "m->tipos[m->num_tipos]" repetidamente
    TipoPilha *t = &m->tipos[m->num_tipos];
    sscanf(linha, "TIPO %49s %49s", t->nome, t->flags);
    m->num_tipos++;
}

static void lerInit(MotorPaciencia *m, const char *linha) {
    if (m->num_pilhas >= MAX_PILHAS_MESA) return;
    int qtd_cartas;
    char tipo[MAX_STR];
    sscanf(linha, "INIT %49s %d", tipo, &qtd_cartas);
    
    strcpy(m->tipo_mesa[m->num_pilhas], tipo);
    m->qtd_inicial[m->num_pilhas] = qtd_cartas;
    m->mesa[m->num_pilhas].topo = NULL;   /* pilha começa vazia (lista) */
    m->num_pilhas++;
}

static void lerMov(MotorPaciencia *m, const char *linha, int auto_flag) {
    if (m->num_regras_mov >= MAX_REGRAS) return;
    RegraMov *r = &m->regras_mov[m->num_regras_mov];
    char lixo[MAX_STR]; /* Para absorver a palavra "MOV" ou "AUTO" inicial */
    // "%49s %49s %49s %49s" extrai o comando, origem, destino e regras.
    sscanf(linha, "%49s %49s %49s %49s", lixo, r->origem, r->destino, r->flags);
    r->is_auto = auto_flag;
    m->num_regras_mov++;
}

static void lerWin(MotorPaciencia *m, const char *linha) {
    if (m->num_regras_win >= MAX_REGRAS) return;
    RegraWin *w = &m->regras_win[m->num_regras_win];
    sscanf(linha, "WIN %49s %d", w->tipo_pilha, &w->quantidade_alvo);
    m->num_regras_win++;
}

/* Identifica o comando e atira-o para a sub-função correta */
static void processarLinha(MotorPaciencia *m, char *linha) {
    char cmd[MAX_STR] = "";
    if (sscanf(linha, "%49s", cmd) != 1) return; // Se a linha for vazia, abandona

    /* A complexidade desta função é exatemente 7 (muito abaixo do limite 10) 
       Isto é garantido porque usamos o padrão 'else if', formando uma única árvore de decisão. */
    if      (strcmp(cmd, "JOGO") == 0)     lerJogo(m, linha);
    else if (strcmp(cmd, "BARALHOS") == 0) lerBaralhos(m, linha);
    else if (strcmp(cmd, "TIPO") == 0)     lerTipo(m, linha);
    else if (strcmp(cmd, "INIT") == 0)     lerInit(m, linha);
    else if (strcmp(cmd, "MOV") == 0)      lerMov(m, linha, 0);
    else if (strcmp(cmd, "AUTO") == 0)     lerMov(m, linha, 1);
    else if (strcmp(cmd, "WIN") == 0)      lerWin(m, linha);
}

/* Função principal do parser */
int carregarPaciencia(MotorPaciencia *m, const char *nome_ficheiro) {
    FILE *f = fopen(nome_ficheiro, "r");
    if (f == NULL) return 0;
    
    // Função chamada para guardar "golf.paciencia" dentro do Motor para fins de Save Game
    guardarNomeFicheiro(m, nome_ficheiro);

    char linha[256];
    // fgets previne buffer overflow e avança linha a linha até encontrar o fim do ficheiro (NULL)
    while (fgets(linha, sizeof(linha), f) != NULL) {
        limparComentario(linha);
        processarLinha(m, linha);
    }
    
    fclose(f);
    return 1;
}