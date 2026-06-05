#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "menu.h"

#define PASTA    "paciencias"
#define MAX_FICH 64
#define MAX_NOME 64

// Ir à pasta do jogo e fazer um inventário de todos os ficheiros de paciências disponíveis.
static int listarFicheiros(char nomes[][MAX_NOME], int max) {
    DIR *dir = opendir(PASTA);                                  // Abre a pasta definida em PASTA 
    if (dir == NULL) return 0;                                  // Se a pasta não existir ou não puder ser aberta, devolve 0.
    
    int n = 0;                                                  // Contador de ficheiros lidos e guardados.
    struct dirent *ent;                                         // Estrutura que guarda a informação de cada ficheiro lido.
    
    while ((ent = readdir(dir)) != NULL && n < max)             // Lê o próximo ficheiro até a pasta acabar (NULL) ou atingir o limite do array (max).
        if (ent->d_name[0] != '.') {                            // Ignora pastas pai/atual (".", "..") e ficheiros ocultos (ex: ".git").
            strcpy(nomes[n], ent->d_name);                      // Copia o nome do ficheiro válido para a matriz 'nomes'.
            n++;                                                // Incrementa o contador de ficheiros guardados.
        }
        
    closedir(dir);                                              // Fecha a stream do diretório para libertar recursos do Sistema Operativo.
    return n;                                                   // Devolve o total de ficheiros listados com sucesso.
}

// Mostra o menu numerado das paciências disponíveis.
static void mostrarMenu(char nomes[][MAX_NOME], int n) {
    printf("\n=== Paciencias disponiveis ===\n");               // Imprime o cabeçalho do menu.
    for (int i = 0; i < n; i++)                                 // Percorre a lista de nomes guardados no array.
        printf("  %d) %s\n", i + 1, nomes[i]);                  // Imprime cada ficheiro numerado (começando no número 1).
    printf("Escolha (1-%d): ", n);                              // Pede ao utilizador para escolher um número dentro das opções disponíveis.
}

// Ler a escolha do jogador e garantir que ele não introduz dados inválidos que possam rebentar o jogo. (Aqui n queremos crashes ah poisssssss!!!)
static int lerEscolha(int n) {
    char buf[16];                                               // Buffer temporário para guardar o que o utilizador escreve no teclado.
    int op = 0;                                                 // Variável para guardar o número convertido.
    if (fgets(buf, sizeof(buf), stdin) == NULL) return -1;      // Lê a linha inteira do teclado de forma segura. Se falhar, devolve -1.
    if (sscanf(buf, "%d", &op) != 1)            return -1;      // Extrai um número (%d) do texto. Se falhar (ex: utilizou letras), devolve -1.
    if (op < 1 || op > n)                       return -1;      // Valida se o número escolhido não existe no menu. Se estiver fora dos limites, devolve -1.
    return op - 1;                                             
}

// É a função principal e entrega o resultado final ao motor de jogo.
int escolherPaciencia(char *caminho) {
    char nomes[MAX_FICH][MAX_NOME];                             // Matriz na memória (Stack) para guardar até MAX_FICH nomes com MAX_NOME letras.
    
    int n = listarFicheiros(nomes, MAX_FICH);                   // Preenche a matriz com os ficheiros da pasta e guarda a quantidade em 'n'.
    if (n == 0) return 0;                                       // Se não encontrou ficheiros, a função falha e devolve 0.
    
    mostrarMenu(nomes, n);                                      // Imprime a lista numerada na consola para o jogador ver.
    
    int i = lerEscolha(n);                                      // Fica à espera que o jogador digite uma opção válida.
    if (i < 0) return 0;                                        // Se a entrada for inválida, cancela o processo e devolve 0.
    
    strcpy(caminho, PASTA);                                     // Inicia a string final copiando o nome da pasta raiz (ex: "paciencias").
    strcat(caminho, "/");                                       // Junta a barra de separação de diretório (ex: "paciencias/").
    strcat(caminho, nomes[i]);                                  // Junta o nome do ficheiro selecionado (ex: "paciencias/golf.paciencia").
    
    return 1;                                                   
}
