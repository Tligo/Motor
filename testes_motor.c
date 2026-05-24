/**
 * @file testes_motor.c
 * @brief Testes CUnit para a lógica de movimentos do motor (regras.c).
 *
 * Cada teste constrói um MotorPaciencia LOCAL (sem variáveis globais) com
 * pilhas (listas ligadas), cartas e regras MOV, e verifica a lógica contra os
 * jogos-exemplo do enunciado (Golf, Simple Simon, FreeCell) e casos sintéticos.
 */

#include <string.h>
#include "regras.h"
#include "CUnit/Basic.h"

/* ── Auxiliares de construção de estado ── */

// Zera tudo (apontadores das listas a NULL) — base limpa e segura para o undo.
static void reset(MotorPaciencia *m) {
    memset(m, 0, sizeof *m);
    m->hist_topo = -1;   /* -1 = histórico vazio (igual ao inicializarMotor) */
}
static void novaPilha(MotorPaciencia *m, const char *tipo) {
    strcpy(m->tipo_mesa[m->num_pilhas], tipo);
    m->mesa[m->num_pilhas].topo = NULL;
    m->num_pilhas++;
}
static void push(MotorPaciencia *m, int p, int valor, int naipe) {
    Cartas c;
    c.valor = valor;
    c.naipe = naipe;
    empilhar(&m->mesa[p], c);
}
// Altera a carta no nível i (0 = fundo) de uma pilha (para variar um teste).
static void setCarta(MotorPaciencia *m, int p, int i, int valor, int naipe) {
    int passos = alturaPilha(&m->mesa[p]) - 1 - i;
    No *atual  = m->mesa[p].topo;
    while (passos > 0) { atual = atual->prox; passos--; }
    atual->carta.valor = valor;
    atual->carta.naipe = naipe;
}
static void addMov(MotorPaciencia *m, const char *o, const char *d, const char *f) {
    RegraMov *r = &m->regras_mov[m->num_regras_mov++];
    strcpy(r->origem, o); strcpy(r->destino, d); strcpy(r->flags, f);
    r->is_auto = 0;
}
static void addAuto(MotorPaciencia *m, const char *o, const char *d, const char *f) {
    addMov(m, o, d, f);
    m->regras_mov[m->num_regras_mov - 1].is_auto = 1;
}
static void addWin(MotorPaciencia *m, const char *tipo, int qtd) {
    RegraWin *w = &m->regras_win[m->num_regras_win++];
    strcpy(w->tipo_pilha, tipo); w->quantidade_alvo = qtd;
}
static void addTipo(MotorPaciencia *m, const char *nome, const char *flags) {
    strcpy(m->tipos[m->num_tipos].nome, nome);
    strcpy(m->tipos[m->num_tipos].flags, flags);
    m->num_tipos++;
}
// Empilha um run completo K,Q,...,A do mesmo naipe (Ás fica no topo).
static void pushRunKA(MotorPaciencia *m, int p, int naipe) {
    for (int v = 13; v >= 1; v--) push(m, p, v, naipe);
}

int init_suite(void)  { return 0; }
int clean_suite(void) { return 0; }

/* ── Golf: MOV STOCK DESCARTE * ; MOV TAB DESCARTE ~ ── */
static void setGolf(MotorPaciencia *m) {
    reset(m);
    novaPilha(m, "TAB"); novaPilha(m, "DESCARTE"); novaPilha(m, "STOCK");
    addMov(m, "STOCK", "DESCARTE", "*"); addMov(m, "TAB", "DESCARTE", "~");
    push(m, 0, 7, 0); push(m, 1, 8, 1); push(m, 2, 5, 2);
}
static void teste_golf_tilde_valido(void) {
    MotorPaciencia m; setGolf(&m);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 1); /* 7 sobre 8 */
}
static void teste_golf_tilde_invalido(void) {
    MotorPaciencia m; setGolf(&m);
    setCarta(&m, 1, 0, 9, 1);                          /* descarte passa a 9 */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 0);
}
static void teste_golf_estrela(void) {
    MotorPaciencia m; setGolf(&m);
    CU_ASSERT_EQUAL(validarMovimento(&m, 2, 1, 1), 1); /* STOCK->DESC sempre */
}
static void teste_golf_sem_regra(void) {
    MotorPaciencia m; setGolf(&m);
    CU_ASSERT_EQUAL(validarMovimento(&m, 1, 0, 1), 0); /* DESC->TAB nao existe */
}

/* ── Simple Simon: < ; +[m< ; V ; +[mV ── */
static void setSimon(MotorPaciencia *m) {
    reset(m);
    novaPilha(m, "TAB"); novaPilha(m, "TAB"); novaPilha(m, "TAB");
    addMov(m, "TAB", "TAB", "<");  addMov(m, "TAB", "TAB", "+[m<");
    addMov(m, "TAB", "TAB", "V");  addMov(m, "TAB", "TAB", "+[mV");
    push(m, 0, 8, 0); push(m, 0, 7, 0); push(m, 0, 6, 0); push(m, 1, 9, 2);
}
static void teste_simon_sequencia_mesmo_naipe(void) {
    MotorPaciencia m; setSimon(&m);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 3), 1); /* 8-7-6 ♠ sobre 9 */
}
static void teste_simon_sequencia_para_vazia(void) {
    MotorPaciencia m; setSimon(&m);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 2, 3), 1); /* +[mV */
}
static void teste_simon_sequencia_naipes_mistos(void) {
    MotorPaciencia m; setSimon(&m);
    setCarta(&m, 0, 1, 7, 1);                          /* quebra o mesmo naipe */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 3), 0);
}
static void teste_simon_sem_mais_de_uma_sem_flag(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "TAB"); novaPilha(&m, "TAB"); addMov(&m, "TAB", "TAB", "<");
    push(&m, 0, 7, 0); push(&m, 1, 8, 1);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 1); /* uma carta ok */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 2), 0); /* n=2 sem '+' */
}

/* ── FreeCell: D< ; aV ; M> ── */
static void setFreecell(MotorPaciencia *m) {
    reset(m);
    novaPilha(m, "TAB"); novaPilha(m, "TAB"); novaPilha(m, "FUND");
    addMov(m, "TAB", "TAB", "D<"); addMov(m, "TAB", "FUND", "aV");
    addMov(m, "TAB", "FUND", "M>");
    push(m, 0, 6, 0); push(m, 1, 7, 1);
}
static void teste_freecell_tableau_cor_alternada(void) {
    MotorPaciencia m; setFreecell(&m);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 1); /* 6♠ sobre 7♥ */
    setCarta(&m, 1, 0, 7, 0);                          /* 7♠ mesma cor */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 0);
}
static void teste_freecell_fundacao_as(void) {
    MotorPaciencia m; setFreecell(&m);
    setCarta(&m, 0, 0, 1, 3);                          /* A♣ no topo do TAB 0 */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 2, 1), 1); /* aV */
}
static void teste_freecell_fundacao_sobe_naipe(void) {
    MotorPaciencia m; setFreecell(&m);
    push(&m, 2, 1, 3);                                 /* FUND topo A♣ */
    setCarta(&m, 0, 0, 2, 3);                          /* 2♣ */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 2, 1), 1); /* M> */
    setCarta(&m, 0, 0, 2, 1);                          /* 2♥ naipe errado */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 2, 1), 0);
}

/* ── Sintéticos: cobertura das restantes flags ── */
static void teste_flag_crescente_e_alternancia(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "A"); novaPilha(&m, "B");
    addMov(&m, "A", "B", "+]"); addMov(&m, "A", "B", "+x"); addMov(&m, "A", "B", "+d");
    push(&m, 0, 5, 0); push(&m, 0, 6, 0);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 2), 1); /* 5,6 crescente (]) */
    setCarta(&m, 0, 1, 6, 1);                          /* ♠,♥ alternados */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 2), 1); /* x e d */
}
static void teste_flag_X_naipe_diferente_destino(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "A"); novaPilha(&m, "B"); addMov(&m, "A", "B", "X");
    push(&m, 0, 5, 0); push(&m, 1, 9, 1);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 1); /* ♠ vs ♥ */
    setCarta(&m, 1, 0, 9, 0);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 0); /* ♠ vs ♠ */
}
static void teste_flag_reis(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "A"); novaPilha(&m, "B"); addMov(&m, "A", "B", "+K");
    push(&m, 0, 13, 0); push(&m, 0, 5, 1);             /* fundo K♠, topo 5♥ */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 2), 1);
    setCarta(&m, 0, 0, 12, 0);                         /* fundo Q */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 2), 0);
}

/* ── Execução de movimentos ── */
static void teste_executar_aplica_jogada(void) {
    MotorPaciencia m; setSimon(&m);
    int alt_destino = alturaPilha(&m.mesa[1]);
    CU_ASSERT_EQUAL(executarMovimento(&m, 0, 1, 3), 1);
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[1]), alt_destino + 3);
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[0]), 0);       /* origem ficou vazia */
}
static void teste_executar_invalido_nao_altera(void) {
    MotorPaciencia m; setSimon(&m);
    int a0 = alturaPilha(&m.mesa[0]), a1 = alturaPilha(&m.mesa[1]);
    CU_ASSERT_EQUAL(executarMovimento(&m, 0, 1, 1), 0); /* 6 sobre 9: invalido */
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[0]), a0);
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[1]), a1);
}
static void teste_movimento_auto_escolhe_sequencia(void) {
    MotorPaciencia m; setSimon(&m);   /* pilha0: 8-7-6 ♠ ; pilha1 topo 9 */
    CU_ASSERT_EQUAL(executarMovimentoAuto(&m, 0, 1), 1); /* move sem indicar n */
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[0]), 0);         /* moveu a sequencia toda */
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[1]), 4);         /* 1 + 3 cartas */
}

/* ── AUTO ── */
static void teste_auto_completa_fundacao(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "TAB"); novaPilha(&m, "FUND");
    addAuto(&m, "TAB", "FUND", "+[mKaV");
    pushRunKA(&m, 0, 0);
    aplicarAutomaticos(&m);
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[0]), 0);   /* TAB esvaziou      */
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[1]), 13);  /* FUND ficou com 13 */
}

/* ── WIN ── */
static void teste_win_tab_vazio(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "TAB"); novaPilha(&m, "TAB");
    addWin(&m, "TAB", 0);
    CU_ASSERT_EQUAL(verificarVitoria(&m), 1); /* ambas vazias */
    push(&m, 0, 5, 0);
    CU_ASSERT_EQUAL(verificarVitoria(&m), 0); /* uma com carta */
}
static void teste_win_conjuncao(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "FOO"); novaPilha(&m, "BAR");
    addWin(&m, "FOO", 1); addWin(&m, "BAR", 0);
    push(&m, 0, 5, 0);
    CU_ASSERT_EQUAL(verificarVitoria(&m), 1);
    push(&m, 1, 9, 0);
    CU_ASSERT_EQUAL(verificarVitoria(&m), 0);
}

/* ── Flag de tipo '1': pilha com no máximo 1 carta ── */
static void teste_flag_capacidade_um(void) {
    MotorPaciencia m; reset(&m);
    addTipo(&m, "TAB", "=");
    addTipo(&m, "CELL", "1=");
    novaPilha(&m, "TAB"); novaPilha(&m, "CELL");
    addMov(&m, "TAB", "CELL", "*");
    push(&m, 0, 5, 0); push(&m, 0, 6, 0);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 1); /* 1 carta p/ CELL vazia */
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 2), 0); /* 2 cartas excede o '1' */
    push(&m, 1, 9, 0);
    CU_ASSERT_EQUAL(validarMovimento(&m, 0, 1, 1), 0); /* CELL cheia: rejeita */
}

/* ── Dica ── */
static void teste_dica_encontra_jogada(void) {
    MotorPaciencia m; setSimon(&m);
    int o = -1, d = -1, n = -1;
    CU_ASSERT_EQUAL(procurarDica(&m, &o, &d, &n), 1);
    CU_ASSERT(o >= 0 && d >= 0 && n >= 1);
    CU_ASSERT_EQUAL(validarMovimento(&m, o, d, n), 1); /* a jogada sugerida é válida */
}
static void teste_dica_sem_jogada(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "TAB"); novaPilha(&m, "TAB");
    addMov(&m, "TAB", "TAB", "<");
    push(&m, 0, 5, 0); push(&m, 1, 9, 0);
    int o, d, n;
    CU_ASSERT_EQUAL(procurarDica(&m, &o, &d, &n), 0);
}

/* ── Undo ── */
static void teste_undo_repoe_estado(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "TAB"); novaPilha(&m, "TAB");
    addMov(&m, "TAB", "TAB", "<");
    push(&m, 0, 7, 0); push(&m, 1, 8, 1);
    guardarHistorico(&m);
    executarMovimento(&m, 0, 1, 1);
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[0]), 0);   /* origem ficou vazia */
    CU_ASSERT_EQUAL(desfazerJogada(&m), 1);
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[0]), 1);   /* 7 voltou */
    CU_ASSERT_EQUAL(alturaPilha(&m.mesa[1]), 1);   /* 8 continua sozinho */
}
static void teste_undo_vazio(void) {
    MotorPaciencia m; reset(&m);
    novaPilha(&m, "TAB");
    CU_ASSERT_EQUAL(desfazerJogada(&m), 0);    /* sem histórico */
}

/* ── Registo e execução ── */
static void registarSuiteJogos(void) {
    CU_pSuite s = CU_add_suite("jogos-exemplo", init_suite, clean_suite);
    CU_add_test(s, "golf ~ valido",        teste_golf_tilde_valido);
    CU_add_test(s, "golf ~ invalido",      teste_golf_tilde_invalido);
    CU_add_test(s, "golf *",               teste_golf_estrela);
    CU_add_test(s, "golf sem regra",       teste_golf_sem_regra);
    CU_add_test(s, "simon seq naipe",      teste_simon_sequencia_mesmo_naipe);
    CU_add_test(s, "simon seq vazia",      teste_simon_sequencia_para_vazia);
    CU_add_test(s, "simon naipes mistos",  teste_simon_sequencia_naipes_mistos);
    CU_add_test(s, "simon n=2 sem +",      teste_simon_sem_mais_de_uma_sem_flag);
}
static void registarSuiteFlags(void) {
    CU_pSuite s = CU_add_suite("flags", init_suite, clean_suite);
    CU_add_test(s, "freecell D<",          teste_freecell_tableau_cor_alternada);
    CU_add_test(s, "freecell aV",          teste_freecell_fundacao_as);
    CU_add_test(s, "freecell M>",          teste_freecell_fundacao_sobe_naipe);
    CU_add_test(s, "crescente/alternado",  teste_flag_crescente_e_alternancia);
    CU_add_test(s, "flag X",               teste_flag_X_naipe_diferente_destino);
    CU_add_test(s, "flag K",               teste_flag_reis);
    CU_add_test(s, "flag 1 capacidade",    teste_flag_capacidade_um);
    CU_add_test(s, "executar jogada",      teste_executar_aplica_jogada);
    CU_add_test(s, "executar invalido",    teste_executar_invalido_nao_altera);
    CU_add_test(s, "movimento auto",       teste_movimento_auto_escolhe_sequencia);
}
static void registarSuiteAutoWin(void) {
    CU_pSuite s = CU_add_suite("auto-win", init_suite, clean_suite);
    CU_add_test(s, "auto completa fundacao", teste_auto_completa_fundacao);
    CU_add_test(s, "win tab vazio",          teste_win_tab_vazio);
    CU_add_test(s, "win conjuncao",          teste_win_conjuncao);
    CU_add_test(s, "dica encontra jogada",   teste_dica_encontra_jogada);
    CU_add_test(s, "dica sem jogada",        teste_dica_sem_jogada);
    CU_add_test(s, "undo repoe estado",      teste_undo_repoe_estado);
    CU_add_test(s, "undo vazio",             teste_undo_vazio);
}
int main(void) {
    if (CU_initialize_registry() != CUE_SUCCESS) return CU_get_error();
    registarSuiteJogos();
    registarSuiteFlags();
    registarSuiteAutoWin();
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
