CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

# Nomes dos executáveis de acordo com a especificação
TARGET_SERVER = meu_servidor
TARGET_CLIENT = meu_navegador

all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Regra para construir o servidor
$(TARGET_SERVER): servidor.c
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) servidor.c

# Regra para construir o cliente
$(TARGET_CLIENT): cliente.c
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) cliente.c

clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT)