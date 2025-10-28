CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g

# Nomes dos executáveis de acordo com a especificação
TARGET_SERVER = meu_servidor
TARGET_CLIENT = meu_navegador

all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Regra para construir o servidor
# ATUALIZADO: Aponta para o arquivo dentro da pasta Servidor/
$(TARGET_SERVER): Servidor/servidor.c
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) Servidor/servidor.c

# Regra para construir o cliente
# ATUALIZADO: Aponta para o arquivo dentro da pasta cliente/
$(TARGET_CLIENT): cliente/cliente.c
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) cliente/cliente.c

clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT)