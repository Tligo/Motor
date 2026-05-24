#ifndef INTERFACE_H
#define INTERFACE_H

#include "motor.h"

/* Cores ANSI — exatamente as mesmas do Golf/Simple Simon (sem o "1;" bright) */
#define COR_VERMELHA "\033[31m"
#define COR_ROXA     "\033[35m"
#define COR_VERDE    "\033[32m"
#define COR_RESET    "\033[0m"

void mostrarMesa(MotorPaciencia *m);

#endif