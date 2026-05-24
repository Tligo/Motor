#ifndef MENU_H
#define MENU_H

/* Lista os ficheiros da pasta "paciencias", mostra-os numerados e lê a
 * escolha do utilizador. Escreve o caminho completo do ficheiro escolhido
 * (ex.: "paciencias/golf.paciencia") em 'caminho' (com capacidade 'tam').
 * Devolve 1 se uma paciência foi escolhida, 0 caso contrário. */
int escolherPaciencia(char *caminho);

#endif
