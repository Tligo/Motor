#include <string.h>
#include "regras.h"

/* ── Auxiliares sobre cartas e pilhas ── */

// Cor da carta: 0=preto (Espadas/Paus), 1=vermelho (Copas/Ouros).
int corCarta(Cartas c) {
    return (c.naipe == 1 || c.naipe == 2) ? 1 : 0;
}

/* Carta de FUNDO do bloco de n cartas do topo de o: a mais profunda do bloco
 * (a que vai encostar no destino). É a carta no nível (altura - n). */
static Cartas cartaFundoBloco(MotorPaciencia *m, int o, int n) {
    return cartaNivel(&m->mesa[o], alturaPilha(&m->mesa[o]) - n);
}

// Naipe (usarCor=0) ou cor (usarCor=1) de uma carta — para reaproveitar ciclos.
static int naipeOuCor(Cartas c, int usarCor) {
    if (usarCor) return corCarta(c);
    return c.naipe;
}

/* ── Verificações internas ao bloco a mover (níveis base..topo) ── */

/* Valores consecutivos no bloco. passo=-1 → decrescente ('['),
 * passo=+1 → crescente (']'), do fundo para o topo. */
static int blocoConsecutivo(MotorPaciencia *m, int o, int n, int passo) {
    int alt  = alturaPilha(&m->mesa[o]);
    int base = alt - n;
    int ok   = 1;
    for (int i = base; i < alt - 1; i++)
        if (cartaNivel(&m->mesa[o], i + 1).valor != cartaNivel(&m->mesa[o], i).valor + passo)
            ok = 0;
    return ok;
}

// Todas as cartas do bloco com o mesmo naipe (usarCor=0) ou cor (usarCor=1).
static int blocoUniforme(MotorPaciencia *m, int o, int n, int usarCor) {
    int alt  = alturaPilha(&m->mesa[o]);
    int ok   = 1;
    // Padrão de verificação: Se alguma carta for diferente da seguinte, a flag ok "quebra".
    for (int i = alt - n; i < alt - 1; i++)
        if (naipeOuCor(cartaNivel(&m->mesa[o], i), usarCor) !=
            naipeOuCor(cartaNivel(&m->mesa[o], i + 1), usarCor))
            ok = 0;
    return ok;
}

// Bloco com naipes (usarCor=0) ou cores (usarCor=1) alternados.
static int blocoAlternado(MotorPaciencia *m, int o, int n, int usarCor) {
    int alt  = alturaPilha(&m->mesa[o]);
    int ok   = 1;
    for (int i = alt - n; i < alt - 1; i++)
        if (naipeOuCor(cartaNivel(&m->mesa[o], i), usarCor) ==
            naipeOuCor(cartaNivel(&m->mesa[o], i + 1), usarCor))
            ok = 0;
    return ok;
}

// Flags que descrevem a ordenação/uniformidade do bloco a mover.
static int checkBloco(MotorPaciencia *m, int o, int n, char f) {
    if (f == '[') return blocoConsecutivo(m, o, n, -1);
    if (f == ']') return blocoConsecutivo(m, o, n,  1);
    if (f == 'm') return blocoUniforme(m, o, n, 0);
    if (f == 'c') return blocoUniforme(m, o, n, 1);
    if (f == 'x') return blocoAlternado(m, o, n, 0);
    if (f == 'd') return blocoAlternado(m, o, n, 1);
    // Se a flag recebida for ex: '<', ela não pertence a esta categoria estrutural.
    // Retorna-se 1 para que o validador possa continuar para a função 'checkComparacao'.
    return 1; 
}

/* ── Flags de comparação entre o bloco e a carta de topo do destino ── */

// 1 se f é uma flag que compara com a carta de topo do destino.
static int eComparacao(char f) {
    return f=='<'||f=='>'||f=='~'||f=='M'||f=='X'||f=='C'||f=='D';
}

// Aplica a comparação f entre a carta a (fundo do bloco) e b (topo do destino).
static int aplicaComparacao(char f, Cartas a, Cartas b) {
    if (f == '<') return a.valor == b.valor - 1;
    if (f == '>') return a.valor == b.valor + 1;
    if (f == '~') return a.valor == b.valor - 1 || a.valor == b.valor + 1;
    if (f == 'M') return a.naipe == b.naipe;
    if (f == 'X') return a.naipe != b.naipe;
    if (f == 'C') return corCarta(a) == corCarta(b);
    return corCarta(a) != corCarta(b); /* 'D' */
}

// Verifica V (destino vazio) e as flags de comparação com o topo do destino.
static int checkComparacao(MotorPaciencia *m, int o, int d, int n, char f) {
    // Flag de vazio (V): Apenas deixa colocar se o destino estiver sem cartas.
    if (f == 'V')                return pilhaVazia(&m->mesa[d]);
    if (!eComparacao(f))         return 1;          /* não é desta categoria */
    if (pilhaVazia(&m->mesa[d])) return 0;          /* comparação precisa de carta */
    return aplicaComparacao(f, cartaFundoBloco(m, o, n), cartaTopo(&m->mesa[d]));
}

/* ── Flags sobre Ás/Rei no topo (a/k) ou no fundo (A/K) do bloco ── */

static int checkCarta(MotorPaciencia *m, int o, int n, char f) {
    if (f == 'a') return cartaTopo(&m->mesa[o]).valor == 1;
    if (f == 'k') return cartaTopo(&m->mesa[o]).valor == 13;
    if (f == 'A') return cartaFundoBloco(m, o, n).valor == 1;
    if (f == 'K') return cartaFundoBloco(m, o, n).valor == 13;
    return 1; /* flag não pertence a esta categoria */
}

// Valida uma única flag, combinando as três categorias.
static int validarUmaFlag(MotorPaciencia *m, int o, int d, int n, char f) {
    return checkBloco(m, o, n, f)
        && checkComparacao(m, o, d, n, f)
        && checkCarta(m, o, n, f);
}

/* Valida TODAS as flags de um comando MOV (conjunção).
 * '*' = sem restrições estruturais; '+' habilita mover sequências (n>1).
 * NOTA: '+' é verificado ANTES de '*' para que regras como
 * "MOV STOCK DESCARTE *" continuem a limitar-se a 1 carta de cada vez
 * (o que faz sentido para o baralho do Golf, p.ex.). Para mover várias
 * tem de existir explicitamente o '+'. */
static int validarFlags(MotorPaciencia *m, int o, int d, int n, const char *flags) {
    if (!strchr(flags, '+') && n > 1) return 0; // Sem '+' obriga a n=1, mesmo com '*'
    if (strchr(flags, '*'))           return 1; // '*' dispensa as restantes verificações
    int ok = 1;
    for (int i = 0; flags[i] != '\0'; i++)
        if (flags[i] != '+' && flags[i] != '*' &&
            !validarUmaFlag(m, o, d, n, flags[i]))
            ok = 0;
    return ok;
}

/* ── Validação de movimento ── */

// Pré-condições estruturais (mesma pilha, índices, nº de cartas suficiente).
static int movimentoBasicoValido(MotorPaciencia *m, int o, int d, int n) {
    if (o == d)                                                    return 0;
    if (o < 0 || d < 0 || o >= m->num_pilhas || d >= m->num_pilhas) return 0;
    if (n < 1)                                                     return 0;
    if (alturaPilha(&m->mesa[o]) < n)                              return 0; // Impede tentar mover mais cartas do que a pilha tem
    return 1;
}

// 1 se os tipos da regra correspondem aos tipos das pilhas origem/destino.
static int correspondeTipos(MotorPaciencia *m, RegraMov *r, int o, int d) {
    return strcmp(r->origem,  m->tipo_mesa[o]) == 0
        && strcmp(r->destino, m->tipo_mesa[d]) == 0;
}

/* Respeita o limite do tipo de pilha com flag '1' (no máximo 1 carta):
   o destino tem de ficar vazio + 1 carta. Sem '1', não há limite. */
static int respeitaCapacidade(MotorPaciencia *m, int d, int n) {
    if (strchr(flagsDoTipo(m, m->tipo_mesa[d]), '1') == NULL) return 1;
    return pilhaVazia(&m->mesa[d]) && n == 1;
}

// 1 se a regra r (MOV ou AUTO) autoriza mover n cartas de o para d.
static int validarRegra(MotorPaciencia *m, RegraMov *r, int o, int d, int n) {
    if (!movimentoBasicoValido(m, o, d, n)) return 0;
    if (!correspondeTipos(m, r, o, d))      return 0;
    if (!respeitaCapacidade(m, d, n))       return 0;
    return validarFlags(m, o, d, n, r->flags);
}

// Procura o primeiro MOV (não-AUTO) aplicável e válido (disjunção de regras).
int validarMovimento(MotorPaciencia *m, int origem, int destino, int n) {
    int valido = 0;
    for (int i = 0; i < m->num_regras_mov && valido == 0; i++)
        if (m->regras_mov[i].is_auto == 0 &&
            validarRegra(m, &m->regras_mov[i], origem, destino, n))
            valido = 1; // Curto-circuito: Encontra uma regra válida e para logo de procurar.
    return valido;
}

/* ── Execução do movimento ── */

/* Retira n cartas do topo de o e coloca-as no topo de d, preservando a ordem.
 * Usa uma pilha auxiliar: ao passar por ela a ordem inverte-se duas vezes. */
static void moverCartas(MotorPaciencia *m, int o, int d, int n) {
    Pilha aux = {NULL}; // Inicializa uma pilha temporária (lista ligada local).
    
    // Algoritmo de preservação da ordem das Listas Ligadas:
    // Ex: Retirar bloco 8,9,10 da pilha. Se mandarmos 1 a 1 para o destino, fica 10,9,8 (invertido).
    // Usando esta Pilha Auxiliar, a sequência inverte a primeira vez no aux e 
    // reverte para a forma original ao entrar no destino!
    for (int i = 0; i < n; i++) empilhar(&aux, desempilhar(&m->mesa[o]));
    for (int i = 0; i < n; i++) empilhar(&m->mesa[d], desempilhar(&aux));
}

// Valida e, se for legal, aplica o movimento na mesa. Devolve 1/0.
int executarMovimento(MotorPaciencia *m, int origem, int destino, int n) {
    if (validarMovimento(m, origem, destino, n) == 0) return 0;
    moverCartas(m, origem, destino, n);
    return 1;
}

/* Como executarMovimento, mas escolhe sozinho o maior nº de cartas válido
   (a sequência movível) — assim o utilizador só indica origem e destino. */
int executarMovimentoAuto(MotorPaciencia *m, int origem, int destino) {
    if (origem < 0 || origem >= m->num_pilhas) return 0;
    for (int n = alturaPilha(&m->mesa[origem]); n >= 1; n--)
        if (executarMovimento(m, origem, destino, n)) return 1;
    return 0;
}

/* ── Movimentos automáticos (AUTO) ── */

/* Tenta a regra AUTO r entre o e d, escolhendo o MAIOR bloco que a satisfaça. */
static int tentarAutoPar(MotorPaciencia *m, RegraMov *r, int o, int d) {
    for (int n = alturaPilha(&m->mesa[o]); n >= 1; n--)
        if (validarRegra(m, r, o, d, n)) {
            moverCartas(m, o, d, n);
            return 1;
        }
    return 0;
}

// Procura, para a origem o, o primeiro destino onde a regra AUTO r se aplica.
static int tentarAutoDestino(MotorPaciencia *m, RegraMov *r, int o) {
    int feito = 0;
    for (int d = 0; d < m->num_pilhas && feito == 0; d++)
        if (tentarAutoPar(m, r, o, d)) feito = 1;
    return feito;
}

// Procura qualquer origem onde a regra AUTO r se aplique.
static int tentarAutoRegra(MotorPaciencia *m, RegraMov *r) {
    int feito = 0;
    for (int o = 0; o < m->num_pilhas && feito == 0; o++)
        if (tentarAutoDestino(m, r, o)) feito = 1;
    return feito;
}

// Aplica uma passagem das regras AUTO; devolve 1 se alguma se aplicou.
static int passoAuto(MotorPaciencia *m) {
    int feito = 0;
    for (int i = 0; i < m->num_regras_mov && feito == 0; i++)
        if (m->regras_mov[i].is_auto && tentarAutoRegra(m, &m->regras_mov[i]))
            feito = 1;
    return feito;
}

// Aplica as regras AUTO em cadeia, até nenhuma mais se aplicar.
void aplicarAutomaticos(MotorPaciencia *m) {
    int mudou = 1;
    // Efeito dominó: Corre os turnos todos as vezes necessárias até a mesa estabilizar.
    while (mudou == 1)
        mudou = passoAuto(m);
}

/* ── Critério de vitória (WIN) ── */

// 1 se TODAS as pilhas do tipo da regra têm exatamente 'quantidade_alvo' cartas.
static int regraWinSatisfeita(MotorPaciencia *m, RegraWin *w) {
    int ok = 1;
    for (int p = 0; p < m->num_pilhas; p++)
        if (strcmp(m->tipo_mesa[p], w->tipo_pilha) == 0 &&
            alturaPilha(&m->mesa[p]) != w->quantidade_alvo)
            ok = 0;
    return ok;
}

// 1 se TODAS as regras WIN se verificam (conjunção). 0 se não houver regras WIN.
int verificarVitoria(MotorPaciencia *m) {
    if (m->num_regras_win == 0) return 0;
    int ganhou = 1;
    for (int i = 0; i < m->num_regras_win; i++)
        if (regraWinSatisfeita(m, &m->regras_win[i]) == 0)
            ganhou = 0; // Se uma das regras de vitória falhar, o jogador ainda não ganhou.
    return ganhou;
}

/* Derrota = não ganhou E não tem nenhuma jogada válida.
   Reutilizamos procurarDica como "existe pelo menos uma jogada legal?" — se
   ela falhar, o jogo está bloqueado e ainda não há WIN, logo é derrota. */
int verificarDerrota(MotorPaciencia *m) {
    if (verificarVitoria(m)) return 0;  // Se ganhou, não é derrota
    int o, d, n;
    return procurarDica(m, &o, &d, &n) == 0; // sem jogadas → derrota
}

/* ── Voltar atrás (undo): fotografias da mesa em listas copiadas ── */

// Guarda uma cópia profunda da mesa antes de uma jogada.
void guardarHistorico(MotorPaciencia *m) {
    if (m->hist_topo < MAX_UNDO - 1) m->hist_topo++;
    for (int p = 0; p < m->num_pilhas; p++) {
        libertarLista(m->historico[m->hist_topo][p].topo); /* limpa o que lá estava */
        // Usamos a função recursiva de cópia do 'cartas.c' para proteger a mesa original.
        m->historico[m->hist_topo][p].topo = copiarLista(m->mesa[p].topo);
    }
}

// Descarta a última fotografia (quando a jogada afinal não se aplicou).
void descartarHistorico(MotorPaciencia *m) {
    if (m->hist_topo < 0) return;
    for (int p = 0; p < m->num_pilhas; p++) {
        // Obrigatório libertar a memória do snapshot descartado
        libertarLista(m->historico[m->hist_topo][p].topo);
        m->historico[m->hist_topo][p].topo = NULL;
    }
    m->hist_topo--;
}

// Restaura a mesa à fotografia anterior (e liberta o estado atual). 1/0.
int desfazerJogada(MotorPaciencia *m) {
    if (m->hist_topo < 0) return 0;
    for (int p = 0; p < m->num_pilhas; p++) {
        libertarLista(m->mesa[p].topo);                       /* liberta estado atual */
        m->mesa[p].topo = m->historico[m->hist_topo][p].topo; /* recupera o anterior  */
        m->historico[m->hist_topo][p].topo = NULL;            /* transfere a posse    */
    }
    m->hist_topo--;
    return 1;
}

/* ── Dica: encontrar a MELHOR jogada válida (reutiliza validarMovimento) ── */

// Procura o maior n que torne legal mover de o para d. 1 se achar (preenche *n).
static int dicaPar(MotorPaciencia *m, int o, int d, int *n) {
    for (int k = alturaPilha(&m->mesa[o]); k >= 1; k--)
        if (validarMovimento(m, o, d, k)) {
            // Guarda o tamanho da sequência validada usando o apontador '*n'
            *n = k;
            return 1;
        }
    return 0;
}

/* Pontuação heurística de uma jogada válida — quanto maior, melhor.
 * Preferências (por ordem de peso):
 *   +500  mesmo naipe entre o FUNDO do bloco e o TOPO do destino
 *         (ex: 7♠ → 8♠ é melhor que 7♥ → 8♠ porque constrói sequência)
 *   +100*n  blocos maiores são mais progresso por jogada
 * O empate fica como está: a iteração escolhe a primeira jogada com o score
 * mais alto, o que é determinístico e fácil de explicar na defesa. */
static int pontuarJogada(MotorPaciencia *m, int o, int d, int n) {
    int score = n * 100; /* preferimos blocos maiores */
    if (!pilhaVazia(&m->mesa[d])) {
        // Bónus por mesmo naipe entre o fundo do bloco e o topo do destino.
        // Sobre pilhas vazias não há comparação possível (não há topo), por isso
        // o bónus naipe não se aplica e fica só o peso do tamanho do bloco.
        Cartas fundo = cartaNivel(&m->mesa[o], alturaPilha(&m->mesa[o]) - n);
        Cartas topo  = cartaTopo(&m->mesa[d]);
        if (fundo.naipe == topo.naipe) score += 500;
    }
    return score;
}

// Procura A MELHOR jogada válida no tabuleiro (maior pontuação).
// 1 se achar (preenche *o,*d,*n); 0 se não existir jogada legal.
int procurarDica(MotorPaciencia *m, int *origem, int *destino, int *n) {
    int melhor_score = -1;
    int achou = 0;

    for (int o = 0; o < m->num_pilhas; o++)
        for (int d = 0; d < m->num_pilhas; d++) {
            int k;
            // Saltamos pares (o,d) sem jogada legal.
            if (dicaPar(m, o, d, &k) == 0) continue;

            int s = pontuarJogada(m, o, d, k);
            if (s > melhor_score) {
                melhor_score = s;
                *origem  = o;
                *destino = d;
                *n       = k;
                achou    = 1;
            }
        }
    return achou;
}
