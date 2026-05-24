#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "menu.h"

#define PASTA    "paciencias"
#define MAX_FICH 64
#define MAX_NOME 64

/* Lê para nomes[] os ficheiros da pasta PASTA (ignora . .. e ocultos).
 * Devolve o número de ficheiros encontrados (0 se a pasta não existir). */
static int listarFicheiros(char nomes[][MAX_NOME], int max) {
    // DIR é um tipo especial de stream (fluxo) para pastas, tal como FILE é para ficheiros.
    DIR *dir = opendir(PASTA);
    if (dir == NULL) return 0; // Proteção: Falha se a pasta "paciencias" não existir.
    
    int n = 0;
    struct dirent *ent;
    
    // readdir lê a próxima entrada da pasta. Quando não houver mais nada, devolve NULL.
    // O curto-circuito (&& n < max) impede que leias mais ficheiros do que o array suporta (Buffer Overflow).
    while ((ent = readdir(dir)) != NULL && n < max)
        // Filtro inteligente do Sistema de Ficheiros:
        // No Linux, as pastas têm sempre o "." (pasta atual) e ".." (pasta mãe). 
        // Além disso, ficheiros como ".git" são ocultos. Tudo isto começa por '.'.
        if (ent->d_name[0] != '.') {        /* ignora ".", ".." e ocultos */
            strcpy(nomes[n], ent->d_name);
            n++;
        }
        
    // Obrigatório fechar a stream do diretório para libertar o "file descriptor" no Sistema Operativo.
    closedir(dir);
    return n;
}

// Mostra o menu numerado das paciências disponíveis.
static void mostrarMenu(char nomes[][MAX_NOME], int n) {
    printf("\n=== Paciencias disponiveis ===\n");
    for (int i = 0; i < n; i++)
        printf("  %d) %s\n", i + 1, nomes[i]);
    printf("Escolha (1-%d): ", n);
}

/* Lê o número escolhido pelo utilizador. Devolve o índice (0..n-1) ou
 * -1 se a entrada for inválida ou fora do intervalo. */
static int lerEscolha(int n) {
    char buf[16];
    int op = 0;

    // O fgets lê a linha inteira de forma segura (máximo 16 chars), e o sscanf extrai o número.
    // Isto impede que o programa rebente se o utilizador escrever "Será que o professor está a ler isto?" em vez de um número.
    if (fgets(buf, sizeof(buf), stdin) == NULL) return -1;
    if (sscanf(buf, "%d", &op) != 1)            return -1;
    
    // Validação de limites: Garante que o jogador não escolhe um ficheiro que não existe na lista.
    if (op < 1 || op > n)                       return -1;
    
    // Devolve (op - 1) para alinhar a escolha do utilizador (ex: escolheu "1") 
    // com o índice real do array em C (que começa em 0).
    return op - 1;
}

int escolherPaciencia(char *caminho) {
    // Alocação de uma matriz de strings na Stack (memória rápida).
    // Ocupa MAX_FICH * MAX_NOME bytes na memória (64 * 64 = 4096 bytes).
    char nomes[MAX_FICH][MAX_NOME];
    
    int n = listarFicheiros(nomes, MAX_FICH);
    if (n == 0) return 0;
    
    mostrarMenu(nomes, n);
    
    int i = lerEscolha(n);
    if (i < 0) return 0;
    
    // Construção do caminho final concatenando strings.
    // Exemplo do que acontece:
    // 1. strcpy põe "paciencias" na string destino.
    // 2. strcat cola a barra "/" -> "paciencias/".
    // 3. strcat cola o nome do ficheiro escolhido -> "paciencias/golf.paciencia".
    strcpy(caminho, PASTA);   /* monta "paciencias/<ficheiro>" */
    strcat(caminho, "/");
    strcat(caminho, nomes[i]);
    
    return 1;
}