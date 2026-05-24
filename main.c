#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "motor.h"
#include "interface.h"
#include "regras.h"
#include "menu.h"
#include "ficheiro.h"

/* Lê uma linha do stdin para buf. Devolve 1 se leu algo, 0 em EOF. */
static int lerLinha(char *buf, int n) {
    // Usar fgets protege contra Buffer Overflow (o clássico erro de injetar mais letras do que o array buf suporta)
    return fgets(buf, n, stdin) != NULL;
}

/* Pausa até o jogador carregar Enter (para ler mensagens antes do ecrã limpar). */
static void pausa(void) {
    char buf[64];
    printf("(Enter para continuar) ");
    // Funciona como um "blocker" na thread principal, congelando o jogo até haver input
    lerLinha(buf, sizeof(buf));
}

/* Trata o comando "move <origem> <destino>" (o nº de cartas é automático).
 * Guarda o estado antes da jogada (para desfazer) e dispara os AUTO. */
static void comandoMover(MotorPaciencia *m, const char *buf) {
    char cmd[16];
    int o = -1, d = -1;
    
    // Parse inteligente: O jogador escreve "move 3 5", e o sscanf distribui os valores
    // pelas variáveis 'cmd', 'o' (origem) e 'd' (destino). Se falhar a extração de 3 partes, dá erro.
    if (sscanf(buf, "%15s %d %d", cmd, &o, &d) < 3) {
        printf("Sintaxe: move <origem> <destino>\n");
        return;
    }
    
    guardarHistorico(m); // Checkpoint obrigatório ANTES de alterar a mesa
    
    if (executarMovimentoAuto(m, o, d) == 0) {
        descartarHistorico(m);             /* jogada inválida: anula o snapshot para poupar memória */
        printf("Jogada invalida.\n");
        pausa();
    } else
        // Reacção em cadeia: O jogo verifica autonomamente se a jogada do utilizador
        // ativou alguma regra automática (ex: mandar cartas para a fundação)
        aplicarAutomaticos(m);             
}

// Desfaz a última jogada ou avisa que não há histórico.
static void processarUndo(MotorPaciencia *m) {
    if (desfazerJogada(m) == 0) printf("Sem jogadas para desfazer.\n");
}

/* Sugere uma jogada válida: marca a origem/destino para serem destacados na
   mesa (reutiliza o procurarDica da lógica). */
static void comandoDica(MotorPaciencia *m) {
    int o, d, n;
    // Se encontrar dica, ativa as variáveis de destaque do Motor. 
    // Na próxima Frame (ciclo), o interface.c vai pintá-las de roxo/verde.
    if (procurarDica(m, &o, &d, &n)) {
        m->dica_origem  = o;
        m->dica_destino = d;
        m->dica_n       = n;
    } else {
        printf("Sem jogadas validas de momento.\n");
        pausa();
    }
}

/* Guarda o jogo num ficheiro (por omissão "jogo_guardado.txt"), para o
 * poderes retomar mais tarde com 'carregar'. */
static void comandoGravar(MotorPaciencia *m, const char *buf) {
    char cmd[16], nome[64] = "jogo_guardado.txt";
    // Tenta extrair um nome de ficheiro opcional (ex: "guardar jogo2"). Se não houver, usa o omissão.
    sscanf(buf, "%15s %63s", cmd, nome);
    if (gravarJogo(m, nome))
        printf("Jogo guardado em '%s'. Usa 'c %s' para voltar a este ponto.\n", nome, nome);
    else
        printf("Nao foi possivel guardar em '%s'.\n", nome);
    pausa();
}

/* Carrega um jogo guardado (por omissão "jogo_guardado.txt"). */
static void comandoCarregar(MotorPaciencia *m, const char *buf) {
    char cmd[16], nome[64] = "jogo_guardado.txt";
    sscanf(buf, "%15s %63s", cmd, nome);
    
    // O carregarJogo substitui o estado inteiro do Motor (m) pelo que estava no disco.
    if (carregarJogo(m, nome))
        printf("Jogo '%s' carregado.\n", nome);
    else
        printf("Nao encontrei o jogo guardado '%s'.\n", nome);
    pausa();
}

// Mostra um ecrã de ajuda com as teclas, descrições e exemplos (incl. save/load).
static void mostrarAjuda(void) {
    // printf multi-linha otimizado do C (strings adjacentes fundem-se automaticamente).
    printf(
        "\n=== COMO JOGAR ===\n"
        "  move <orig> <dest>     - move a carta (ou a sequencia movivel) entre pilhas\n"
        "        ex:  move 0 3     (move o topo/sequencia da pilha 0 para a 3)\n"
        "  d   - dica       (sugere uma jogada valida)\n"
        "  u   - desfazer   (anula a ultima jogada)\n"
        "  g   - guardar [nome]   (grava o jogo;    ex:  g    ou    g partida1)\n"
        "  c   - carregar [nome]  (retoma um jogo;  ex:  c partida1)\n"
        "  a   - ajuda      (mostra esta ajuda)\n"
        "  q   - sair       (termina o jogo)\n"
        "==================\n");
}

// 1 se cmd corresponde à tecla curta ou à palavra completa do comando.
static int ehComando(const char *cmd, const char *tecla, const char *palavra) {
    return strcmp(cmd, tecla) == 0 || strcmp(cmd, palavra) == 0;
}

/* Interpreta o comando lido. Devolve 0 se for para sair, 1 caso contrário. */
static int processarComando(MotorPaciencia *m, const char *buf) {
    char cmd[16] = "";
    sscanf(buf, "%15s", cmd);
    
    // Padrão de encaminhamento (Router): Quebra de complexidade evitando swich-cases longos
    if (ehComando(cmd, "q", "sair")) return 0;
    
    if      (strcmp(cmd, "move") == 0)        comandoMover(m, buf);
    else if (ehComando(cmd, "d", "dica"))     comandoDica(m);
    else if (ehComando(cmd, "u", "desfazer")) processarUndo(m);
    else if (ehComando(cmd, "g", "guardar"))  comandoGravar(m, buf);
    else if (ehComando(cmd, "c", "carregar")) comandoCarregar(m, buf);
    else if (ehComando(cmd, "a", "ajuda"))  { mostrarAjuda(); pausa(); }
    else { printf("Comando desconhecido. Escreve 'a' (ajuda) para ver os comandos.\n"); pausa(); }
    
    return 1;
}

/* Limpa o ecrã (como no Golf/Simple Simon) para cada estado substituir o anterior. */
static void limparEcra(void) {
    // Dá a ilusão de um "frame de jogo" limpo a cada input
    system("clear");
}

/* Mostra a mesa, lê um comando e processa-o. Devolve 0 para terminar.
   O destaque da dica dura apenas este ecrã (é limpo após mostrar a mesa). */
static int jogarTurno(MotorPaciencia *m) {
    char buf[64];
    limparEcra();
    mostrarMesa(m); // Chama o teu motor gráfico
    
    // Apaga a dica (Reset) APÓS o desenho para não ficar congelada no ecrã para sempre
    m->dica_origem = -1; m->dica_destino = -1; m->dica_n = -1;
    
    if (verificarVitoria(m)) { printf("VITORIA! Paciencia resolvida.\n"); return 0; }
    
    printf("Comandos: move <o> <d> | d-dica | u-desfazer | g-guardar | c-carregar | a-ajuda | q-sair\n> ");
    if (lerLinha(buf, sizeof(buf)) == 0) return 0; // Proteção EOF (Ctrl+D)
    
    return processarComando(m, buf);
}

/* Carrega a paciência e prepara a mesa. Devolve 1 em sucesso, 0 em erro. */
static int prepararJogo(MotorPaciencia *m, const char *ficheiro) {
    inicializarMotor(m);
    if (carregarPaciencia(m, ficheiro) == 0) return 0; // Se o parser falhar, aborta a inicialização
    distribuirCartasMesa(m);
    return 1;
}

/* Escolhe e prepara a paciência; imprime o erro adequado. Devolve 1/0. */
static int iniciar(MotorPaciencia *m) {
    char caminho[80];
    limparEcra();                   /* ecrã limpo: só o menu */
    
    // Bloqueia o arranque se não houver jogos na pasta
    if (escolherPaciencia(caminho) == 0) {
        printf("Nenhuma paciencia escolhida (pasta 'paciencias' vazia?).\n");
        return 0;
    }
    if (prepararJogo(m, caminho) == 0) {
        printf("[ERRO] Nao foi possivel abrir %s\n", caminho);
        return 0;
    }
    return 1;
}

int main(void) {
    // Alimenta o relógio interno do PC como "semente" para garantir baralhos aleatórios autênticos
    srand((unsigned int)time(NULL));
    
    MotorPaciencia motor;
    // MEDIDA DE SEGURANÇA: Garante que os apontadores da struct não começam a apontar para
    // lixo de memória RAM deixado por outros programas, evitando Segmentation Faults no arranque.
    memset(&motor, 0, sizeof motor); 
    
    if (iniciar(&motor) == 0) return 1;
    
    limparEcra();                   /* após escolher: só as regras */
    mostrarAjuda();
    pausa();
    
    // O sagrado "Game Loop": O jogo fica preso neste ciclo até o utilizador pedir para sair ('q') ou ganhar.
    int jogar = 1;
    while (jogar == 1)
        jogar = jogarTurno(&motor);
        
    return 0;
}