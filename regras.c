#include <string.h>
#include "regras.h"

/* ── Auxiliares sobre cartas e pilhas ── */

// Cor da carta: 0=preto (Espadas/Paus), 1=vermelho (Copas/Ouros).
int corCarta(Cartas c) {
    return (c.naipe == 1 || c.naipe == 2) ? 1 : 0;
}

// Funçao para descobrir qual é a carta que vamos usar para comparar com as outras (carta do fundo)
static Cartas cartaFundoBloco(MotorPaciencia *m, int o, int n) {
    return cartaNivel(&m->mesa[o], alturaPilha(&m->mesa[o]) - n);
}

// Naipe (usarCor=0) ou cor (usarCor=1) de uma carta para reaproveitar ciclos.
static int naipeOuCor(Cartas c, int usarCor) {
    if (usarCor) return corCarta(c);
    return c.naipe;
}

/* ── Verificações internas ao bloco a mover (níveis base..topo) ── */

// Verifica os valores 
// Valores consecutivos no bloco. passo=-1 → decrescente ('['), passo=+1 → crescente (']'), do fundo para o topo. 
static int blocoConsecutivo(MotorPaciencia *m, int o, int n, int passo) {
    int alt  = alturaPilha(&m->mesa[o]);        // Ex: Se a pilha tem 7 cartas, 'alt' = 7.
    int base = alt - n;    
    int ok   = 1;
    for (int i = base; i < alt - 1; i++) // O ciclo começa na base do bloco e sobe até ao topo.
        
        // Pega no valor da carta de baixo (i) e soma-lhe o 'passo' (-1 ou +1).
        // Se o resultado for DIFERENTE (!=) da carta que está logo acima (i + 1)...
        if (cartaNivel(&m->mesa[o], i + 1).valor != cartaNivel(&m->mesa[o], i).valor + passo)        
            ok = 0;
    return ok;
}
// Verifica os naipes / cores
// Quebra se as cartas forem diferentes
// Todas as cartas do bloco com o mesmo naipe (usarCor=0) ou cor (usarCor=1).
static int blocoUniforme(MotorPaciencia *m, int o, int n, int usarCor) {
    int alt  = alturaPilha(&m->mesa[o]);    // Vai à lista ligada da pilha de origem 'o' e conta quantas cartas lá estão no total.
    int ok   = 1; // Assumimos que o bloco é perfeito e uniforme (1 = Verdadeiro).
    // Se alguma carta for diferente da seguinte, a flag ok "quebra".
    for (int i = alt - n; i < alt - 1; i++)    
        // Ela extrai a cor ou o naipe da carta de baixo (i) e compara com a da carta de cima (i + 1).
        // Se a característica (cor ou naipe) for DIFERENTE (!=) entre as duas cartas vizinhas.
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
        if (naipeOuCor(cartaNivel(&m->mesa[o], i), usarCor) == // Quebra se as cartas forem iguais ao verificar os naipes
            naipeOuCor(cartaNivel(&m->mesa[o], i + 1), usarCor))
            ok = 0;
    return ok;
}

// Flags que descrevem a ordenação/uniformidade do bloco a mover.
static int checkBloco(MotorPaciencia *m, int o, int n, char f) {
    if (f == '[') return blocoConsecutivo(m, o, n, -1);    // (Ex: 10, 9, 8).
    if (f == ']') return blocoConsecutivo(m, o, n,  1);    // (Ex: 8, 9, 10).
    if (f == 'm') return blocoUniforme(m, o, n, 0);        // O bloco tem de ter o mesmo naipe puro? (Flag 'm').
    if (f == 'c') return blocoUniforme(m, o, n, 1);        // O bloco tem de ter a mesma cor? (Flag 'c').
    if (f == 'x') return blocoAlternado(m, o, n, 0);       // O bloco tem de ter naipes alternados? (Flag 'x').
    if (f == 'd') return blocoAlternado(m, o, n, 1);       // O bloco tem de ter cores alternadas? (Flag 'd').
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
    if (f == '<') return a.valor == b.valor - 1;    // Ex: Se destino(b) é um 10, a carta que entra(a) tem de ser um 9.
    if (f == '>') return a.valor == b.valor + 1;    // Ex: Se destino(b) é um 5, a carta que entra(a) tem de ser um 6 
    if (f == '~') return a.valor == b.valor - 1 || a.valor == b.valor + 1;    // A carta 'a' pode ser um valor ABAIXO OU ACIMA da carta 'b' (Adjacente).
    if (f == 'M') return a.naipe == b.naipe;    // Naipe da carta 'a' tem de ser IGUAL ao da carta 'b'.
    if (f == 'X') return a.naipe != b.naipe;    // Naipe da carta 'a' tem de ser DIFERENTE do naipe de 'b'.
    if (f == 'C') return corCarta(a) == corCarta(b);    // A cor da carta 'a' tem de ser IGUAL (Cor) à cor da carta 'b'.
    return corCarta(a) != corCarta(b); // Se chegou até aqui as cores têm de ser diferentes.
}

// Verifica V (destino vazio) e as flags de comparação com o topo do destino.
static int checkComparacao(MotorPaciencia *m, int o, int d, int n, char f) {
    // Apenas deixa colocar se o destino estiver sem cartas.
    if (f == 'V')                return pilhaVazia(&m->mesa[d]);    // Pergunta à função 'pilhaVazia' se a pilha de destino 'd' tem 0 cartas.
    if (!eComparacao(f))         return 1;          // Vê se pertence a familia de 'eComparacao' ( < ou > etc..).
    if (pilhaVazia(&m->mesa[d])) return 0;          // Se a pilha de destino ('d') estiver completamente vazia não há nenhuma carta lá para comparar (prevenir crash).

    /* A operação matemática a fazer (ex: '<')
       cartaFundoBloco: Vai buscar a carta mais profunda do conjunto que o jogador levantou ('o').
       cartaTopo: Vai buscar a carta que está no topo da pilha de destino ('d'). */
    return aplicaComparacao(f, cartaFundoBloco(m, o, n), cartaTopo(&m->mesa[d])); 
}

/* ── Flags sobre Ás/Rei no topo (a/k) ou no fundo (A/K) do bloco ── */

static int checkCarta(MotorPaciencia *m, int o, int n, char f) {
    if (f == 'a') return cartaTopo(&m->mesa[o]).valor == 1;    // O TOPO da pilha de origem (a carta mais exposta) é um Ás (1)?
    if (f == 'k') return cartaTopo(&m->mesa[o]).valor == 13;    // O TOPO da pilha de origem é um Rei (13)?
    if (f == 'A') return cartaFundoBloco(m, o, n).valor == 1;    // O FUNDO DO BLOCO que o jogador tem na mão é um Ás (1)?
    if (f == 'K') return cartaFundoBloco(m, o, n).valor == 13;    // O FUNDO DO BLOCO que o jogador tem na mão é um Rei (13)?
    return 1; // flag não pertence a esta categoria 
}

// Valida uma única flag, combinando as três categorias (passar pelas 3 funçoes).
static int validarUmaFlag(MotorPaciencia *m, int o, int d, int n, char f) {
    return checkBloco(m, o, n, f)
        && checkComparacao(m, o, d, n, f)
        && checkCarta(m, o, n, f);
}

// Analisa uma string completa de regras (ex: "+[<") e verifica se todas passam.
// A função 'strchr' procura a primeira ocorrência de um caractere específico
static int validarFlags(MotorPaciencia *m, int o, int d, int n, const char *flags) {
    if (!strchr(flags, '+') && n > 1) return 0;     // Sem '+' obriga a n=1, mesmo com '*' (se a regra não tem '+', é obrigatório mover 1 a 1)
    if (strchr(flags, '*'))           return 1;     // '*' dispensa as restantes verificações
    int ok = 1;
    for (int i = 0; flags[i] != '\0'; i++)
        if (flags[i] != '+' && flags[i] != '*' &&    // Se a letra que estamos a ler agora não for o '+' nem o '*' manda pra validarFlag
            !validarUmaFlag(m, o, d, n, flags[i]))
            ok = 0;
    return ok;
}

/* ── Validação de movimento ── */

// Pré-condições estruturais (mesma pilha, índices, nº de cartas suficiente).  É pra prevenir crashes
static int movimentoBasicoValido(MotorPaciencia *m, int o, int d, int n) {    
    if (o == d)                                                    return 0;    // Mover da pilha 2 para a pilha 2. 
    if (o < 0 || d < 0 || o >= m->num_pilhas || d >= m->num_pilhas) return 0;    // Se o jogador mandar mover da pilha 10 numa mesa que só tem 7 pilhas.
    if (n < 1)                                                     return 0;    // Um comando com n=0 ou n negativo (ex: mover -2 cartas).
    if (alturaPilha(&m->mesa[o]) < n)                              return 0;  // Impede tentar mover mais cartas do que a pilha tem
    return 1;
}

// Se os tipos da regra correspondem aos tipos das pilhas origem/destino.
static int correspondeTipos(MotorPaciencia *m, RegraMov *r, int o, int d) {
    return strcmp(r->origem,  m->tipo_mesa[o]) == 0   // A função 'strcmp' compara o texto do tipo de origem exigido pela regra (r->origem) com o tipo real da pilha de origem que o jogador escolheu (m->tipo_mesa[o]).
        && strcmp(r->destino, m->tipo_mesa[d]) == 0;  // Faz a mesma comparação de texto, mas agora para a pilha de destino (d).
}

// Garante que não colocas mais cartas do que a zona permite (A regra do '1').
static int respeitaCapacidade(MotorPaciencia *m, int d, int n) {
    if (strchr(flagsDoTipo(m, m->tipo_mesa[d]), '1') == NULL) return 1;    // Se nao encontrar 1 (null) significa que esta zona não tem limites de capacidade restritos
    return pilhaVazia(&m->mesa[d]) && n == 1; // se chegar aqui a pilha tem de estar vazia e so pode mover uma carta
}

// Se a regra r (MOV ou AUTO) autoriza mover n cartas de o para d.
static int validarRegra(MotorPaciencia *m, RegraMov *r, int o, int d, int n) {
    // Testa se passar estas 3 validaçoes
    if (!movimentoBasicoValido(m, o, d, n)) return 0;
    if (!correspondeTipos(m, r, o, d))      return 0;
    if (!respeitaCapacidade(m, d, n))       return 0;
    return validarFlags(m, o, d, n, r->flags);    // Se passar as 3 manda para validarFlags (Ver se naipes / cores / numeros) encaixam-
}

// Procura o primeiro MOV (não-AUTO) aplicável e válido-
int validarMovimento(MotorPaciencia *m, int origem, int destino, int n) {
    int valido = 0;    // Assume que a jogada é ilegal até encontrar uma regra que a aprove.
    for (int i = 0; i < m->num_regras_mov && valido == 0; i++)    // Percorre todas as regras. O ciclo pára assim que 'valido' passar a 1.
        if (m->regras_mov[i].is_auto == 0 &&    // Se a regra não for automática (is_auto == 0) E a função validarRegra aprovar
            validarRegra(m, &m->regras_mov[i], origem, destino, n))
            valido = 1; // aprova a jogada e quebra o ciclo.
    return valido;
}

/* ── Execução do movimento ── */

/* Retira n cartas do topo de o e coloca-as no topo de d, preservando a ordem
   ao passar por ela a ordem inverte-se duas vezes. */
static void moverCartas(MotorPaciencia *m, int o, int d, int n) {
    Pilha aux = {NULL}; // Inicializa uma pilha temporária (lista ligada local).
    
    // Ex: Retirar bloco 8,9,10 da pilha. Se mandarmos 1 a 1 para o destino, fica 10,9,8 (invertido).
    // Usando esta Pilha Auxiliar, a sequência inverte a primeira vez no aux e reverte para a forma original ao entrar no destino!
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
    if (origem < 0 || origem >= m->num_pilhas) return 0;    // Testa a jogada. Se for ilegal, cancela e devolve 0.
    for (int n = alturaPilha(&m->mesa[origem]); n >= 1; n--)    // Se for legal, move efetivamente os nós (cartas) na memória.
        if (executarMovimento(m, origem, destino, n)) return 1;
    return 0;
}

/* ── Movimentos automáticos (AUTO) ── */         // Nao sabemos se é suposto haver os mov automaticos mas ta aqui feito

// Tenta aplicar uma regra AUTO testando qual o tamanho de bloco que consegue mover.
static int tentarAutoPar(MotorPaciencia *m, RegraMov *r, int o, int d) {
    for (int n = alturaPilha(&m->mesa[o]); n >= 1; n--) // Testa primeiro o bloco máximo e desce até 1 carta.
        if (validarRegra(m, r, o, d, n)) {              // Se a regra aprovar mover 'n' cartas
            moverCartas(m, o, d, n);                    // Move as cartas fisicamente na memória.
            return 1;                                   
        }
        
    return 0;                                           // Se tentou todos os tamanhos e nenhum deu, a regra falhou (devolve 0).
}


// Fixa uma origem e testa todas as pilhas para ver qual serve de destino.
static int tentarAutoDestino(MotorPaciencia *m, RegraMov *r, int o) {
    int feito = 0;                                                  // Assume que não conseguiu mover.
    for (int d = 0; d < m->num_pilhas && feito == 0; d++)           // Percorre todas as pilhas da mesa à procura do destino 'd'.
        if (tentarAutoPar(m, r, o, d)) feito = 1;                   // Se a função tentarAutoPar conseguir mover as cartas, feito = 1 e pára o ciclo.
    return feito;                                                   // Devolve 1 se o movimento aconteceu, 0 se falhou todos os destinos.
}


// Pega numa regra AUTO e testa todas as pilhas para ver qual serve de origem.
static int tentarAutoRegra(MotorPaciencia *m, RegraMov *r) {
    int feito = 0;                                                  // Assume que não encontrou origem válida.
    for (int o = 0; o < m->num_pilhas && feito == 0; o++)           // Percorre todas as pilhas da mesa a tentar defini-las como origem 'o'.
        if (tentarAutoDestino(m, r, o)) feito = 1;                  // Manda a origem escolhida para tentarAutoDestino. Se resultar, pára o ciclo.
    return feito;                                                   
}


// Faz uma passagem completa por todas as regras para fazer UM movimento automático.
static int passoAuto(MotorPaciencia *m) {
    int feito = 0;                                                  // Assume que nenhum movimento automático foi feito.
    for (int i = 0; i < m->num_regras_mov && feito == 0; i++)       // Percorre a lista de todas as regras do jogo.
        if (m->regras_mov[i].is_auto && tentarAutoRegra(m, &m->regras_mov[i])) // Se a regra for AUTO (1) E conseguir ser aplicada na mesa
            feito = 1;                                           
    return feito;                                                   // Devolve 1 se fez algum movimento automático, para que o motor volte a chamar o passoAuto.
}


// Aplica movimentos automáticos em cadeia até esgotar todas as possibilidades.
void aplicarAutomaticos(MotorPaciencia *m) {
    int mudou = 1;        
    while (mudou == 1)       
    /* Enquanto a função passoAuto conseguir fazer alterações na mesa
       Continua a chamá-la. Quando ela não mover nada, devolve 0 e o ciclo para. */
        mudou = passoAuto(m);   
}

/* ── Critério de vitória (WIN) ── */

// Verifica se uma regra de vitória específica foi atingida (ex: fundações com 13 cartas).
static int regraWinSatisfeita(MotorPaciencia *m, RegraWin *w) {
    int ok = 1;                                             // Assume que a regra já foi cumprida.
    for (int p = 0; p < m->num_pilhas; p++)                 // Percorre todas as pilhas da mesa.
        if (strcmp(m->tipo_mesa[p], w->tipo_pilha) == 0 &&  // Se a pilha for do tipo exigido (ex: "Fundacao") e numero de cartas nao for o correto, pimba ja falhaste
            alturaPilha(&m->mesa[p]) != w->quantidade_alvo)
            ok = 0;                                         
    return ok;                                              // Devolve 1 se todas as pilhas alvo estiverem perfeitamente cheias.
}

// Junta e verifica todas as condições de vitória do jogo inteiro.
int verificarVitoria(MotorPaciencia *m) {
    if (m->num_regras_win == 0) return 0;                   // Se o jogo não tiver regras WIN no ficheiro, é impossível ganhar.
    int ganhou = 1;                                         
    for (int i = 0; i < m->num_regras_win; i++)             // Percorre a lista de todas as regras WIN.
        if (regraWinSatisfeita(m, &m->regras_win[i]) == 0)  // Se a função acima disser que alguma regra ainda não foi cumprida cancela a vitória.
            ganhou = 0;                                     
    return ganhou;                                          // Devolve 1 (Ganhou o jogo!) se passou por todas as regras, ou 0 se ainda está a jogar.
}

// Verificar a derrota (imagina perder ahahaha)
int verificarDerrota(MotorPaciencia *m) {
    if (verificarVitoria(m)) return 0;  // Se ganhou, não é derrota
    int o, d, n;
    return procurarDica(m, &o, &d, &n) == 0; // sem jogadas tambem derrota
}

/* ── Voltar atrás (undo): fotografias da mesa em listas copiadas ── */

// Guarda uma cópia profunda da mesa antes de uma jogada.
void guardarHistorico(MotorPaciencia *m) {
    if (m->hist_topo < MAX_UNDO - 1) m->hist_topo++;   // Avança o índice do histórico, garantindo que não ultrapassa o limite máximo.
    for (int p = 0; p < m->num_pilhas; p++) {          // Percorre todas as pilhas da mesa atual. 
        libertarLista(m->historico[m->hist_topo][p].topo);  // Limpa o "slot" de histórico atual caso já tivesse memória alocada de um Undo antigo.
        // Usamos a função recursiva de cópia do 'cartas.c' para proteger a mesa original.
        m->historico[m->hist_topo][p].topo = copiarLista(m->mesa[p].topo); // Faz um clone perfeito 
    }
}


// Apaga o último snapshot guardado (usado ao fazer Undo).
void descartarHistorico(MotorPaciencia *m) {
    if (m->hist_topo < 0) return;                      // Se não houver histórico nenhum gravado (índice -1), sai da função.
    for (int p = 0; p < m->num_pilhas; p++) {          // Percorre as pilhas do snapshot de histórico mais recente. 
        // Obrigatório libertar a memória do snapshot descartado
        libertarLista(m->historico[m->hist_topo][p].topo); // Destrói fisicamente os nós da lista ligada para evitar memory leaks.
        m->historico[m->hist_topo][p].topo = NULL;         // Mete o ponteiro a NULL por segurança (evita dangling pointers).
    }
    m->hist_topo--;                                    // Recua o índice, recuando o histórico em uma jogada.
}

// Restaura a mesa à fotografia anterior (e liberta o estado atual). 
int desfazerJogada(MotorPaciencia *m) {
    if (m->hist_topo < 0) return 0;                             // Se não houver histórico gravado, falha e devolve 0.
    for (int p = 0; p < m->num_pilhas; p++) {                   // Percorre todas as pilhas da mesa atual.
        libertarLista(m->mesa[p].topo);                         // Apaga as cartas da mesa atual para evitar memory leaks.
        m->mesa[p].topo = m->historico[m->hist_topo][p].topo;   // Substitui a mesa atual pelo snapshot guardado no histórico.
        m->historico[m->hist_topo][p].topo = NULL;              // Limpa o ponteiro do histórico para não haver duplicação de acessos.
    }
    m->hist_topo--;                                             // Recua o índice do histórico.
    return 1;                                                 
}

/* ── Dica: encontrar a MELHOR jogada válida (reutiliza validarMovimento) ── */

// Procura o maior n que torne legal mover de o para d. 1 se achar (preenche *n).
static int dicaPar(MotorPaciencia *m, int o, int d, int *n) {
    for (int k = alturaPilha(&m->mesa[o]); k >= 1; k--)         // Tenta mover o bloco máximo primeiro e vai descendo até 1 carta.
        if (validarMovimento(m, o, d, k)) {                    
            // Guarda o tamanho da sequência validada usando o apontador '*n'
            *n = k;                                             
            return 1;                                           // Encontrou a melhor jogada! Para de procurar e devolve 1.
        }
    return 0;                                                   // Se tentou todos os tamanhos e nenhum deu, a dica falha (devolve 0).
}

/* Pontuação heurística de uma jogada válida — quanto maior, melhor.
  Preferências (por ordem de peso):
  ex: 7♠ → 8♠ é melhor que 7♥ → 8♠ porque constrói sequência
  +100*n  blocos maiores são mais progresso por jogada. */
static int pontuarJogada(MotorPaciencia *m, int o, int d, int n) {
    int score = n * 100; // preferimos blocos maiores */
    if (!pilhaVazia(&m->mesa[d])) {
        /* Bónus por mesmo naipe entre o fundo do bloco e o topo do destino.
         Sobre pilhas vazias não há comparação possível (não há topo), por isso
         o bónus naipe não se aplica e fica só o peso do tamanho do bloco. */
        Cartas fundo = cartaNivel(&m->mesa[o], alturaPilha(&m->mesa[o]) - n);
        Cartas topo  = cartaTopo(&m->mesa[d]);
        if (fundo.naipe == topo.naipe) score += 500;
    }
    return score;
}

// Procura a melhor jogada válida no tabuleiro (maior pontuação).
// se achar (preenche *o,*d,*n); 0 se não existir jogada legal.
int procurarDica(MotorPaciencia *m, int *origem, int *destino, int *n) {
    
    int melhor_score = -1; // Começa com -1 para que a primeira jogada legal (com pontuação >= 0) assuma logo a liderança.
    int achou = 0;         // Assume inicialmente que não existem jogadas possíveis no tabuleiro.

    for (int o = 0; o < m->num_pilhas; o++)                 // Percorre todas as pilhas da mesa tentando usá-las como origem 'o'.
        for (int d = 0; d < m->num_pilhas; d++) {           // Para cada origem, testa todas as pilhas como destino 'd'.
            
            int k;                                          // Variável temporária que vai receber o número de cartas a mover.
            
            // Saltamos pares (o,d) sem jogada legal.
            if (dicaPar(m, o, d, &k) == 0) continue;        // Se a jogada for ilegal, aborta esta tentativa e salta para o próximo destino.

            int s = pontuarJogada(m, o, d, k);              // Se a jogada for legal, calcula o quão "boa" ela é (os pontos que vale).
            
            if (s > melhor_score) {                        
                melhor_score = s;                           // Se esta jogada tiver uma pontuação superior ao nosso recorde atual atualiza o melhor
                *origem  = o;                               // Guarda a origem vencedora no apontador de saída.
                *destino = d;                               // Guarda o destino vencedor no apontador.
                *n       = k;                               // Guarda o número de cartas vencedor.
                achou    = 1;                               // Regista que encontrou pelo menos uma jogada válida para dar como dica.
            }
        }
        
    return achou; // Devolve 1 se os apontadores foram preenchidos com a melhor dica, ou 0 se o jogo estiver bloqueado.
}
