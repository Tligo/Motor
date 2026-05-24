#include <stdlib.h>
#include "motor.h"

/* Preenche um array com 'num_baralhos' baralhos completos. Devolve o total. */
static int preencherBaralho(Cartas *baralho, int num_baralhos) {
    int k = 0;
    for (int b = 0; b < num_baralhos; b++)
        for (int naipe = 0; naipe < 4; naipe++)
            for (int valor = 1; valor <= 13; valor++) {
                baralho[k].naipe = naipe;
                baralho[k].valor = valor;
                k++;
            }
    return k; // Devolve o número total de cartas geradas (ex: 52, 104, etc.)
}

/* Baralha o array com 200 trocas aleatórias. */
static void misturar(Cartas *baralho, int total) {
    for (int n = 0; n < 200; n++) {
        // rand() % total garante que os índices (i e j) nunca ultrapassam o tamanho do baralho alocado
        int i = rand() % total;
        int j = rand() % total;
        
        // Troca de variáveis clássica (usando a variável temporária 't')
        Cartas t = baralho[i];
        baralho[i] = baralho[j];
        baralho[j] = t;
    }
}

/* Distribui as cartas (empilhando-as) pelas pilhas, conforme os INIT. */
void distribuirCartasMesa(MotorPaciencia *m) {
    int total = m->num_baralhos * 52;
    
    // Alocação Dinâmica Crítica: Como o número de baralhos é variável (lido do ficheiro),
    // temos de pedir memória exata para as cartas todas em runtime (malloc).
    Cartas *baralho = malloc(total * sizeof(Cartas));
    
    preencherBaralho(baralho, m->num_baralhos);
    misturar(baralho, total);

    int k = total - 1; /* próxima carta a distribuir (do fim do array) */
    
    // Ciclo de Distribuição:
    // Percorre todas as pilhas e dá a quantidade exata de cartas exigidas pelo ficheiro INIT.
    for (int i = 0; i < m->num_pilhas; i++)
        // A condição (k >= 0) protege o programa de rebentar caso o ficheiro exija
        // distribuir mais cartas do que aquelas que o baralho gerado tem.
        for (int c = 0; c < m->qtd_inicial[i] && k >= 0; c++)
            empilhar(&m->mesa[i], baralho[k--]);

    // Limpeza: Como este baralho gigante só serve para distribuir as cartas pelas pilhas 
    // na primeira ronda, apagamos-o da memória para não deixar Memory Leaks. As cartas agora
    // vivem dentro das Listas Ligadas (m->mesa).
    free(baralho);
}