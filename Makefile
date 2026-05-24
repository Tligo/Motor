# Compilador e flags
CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

# Executável principal
TARGET = motor
OBJS   = main.o parser.o setup.o interface.o regras.o menu.o ficheiro.o cartas.o

# Executável de testes da lógica (precisa do CUnit: -lcunit)
TARGET_TESTES = testes
OBJS_TESTES   = testes_motor.o regras.o parser.o cartas.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compilação genérica
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra de testes (precisa do CUnit: -lcunit)
$(TARGET_TESTES): $(OBJS_TESTES)
	$(CC) $(CFLAGS) -o $(TARGET_TESTES) $(OBJS_TESTES) -lcunit

test: $(TARGET_TESTES)
	./$(TARGET_TESTES)

clean:
	rm -f $(OBJS) $(OBJS_TESTES) $(TARGET) $(TARGET_TESTES)
