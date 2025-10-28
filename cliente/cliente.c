#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <libgen.h>
#include <stdbool.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

void parse_url(const char *url, char *host, int *port, char *path) {
    char url_copy[1024];
    strncpy(url_copy, url, 1023);
    url_copy[1023] = '\0';

    char *hostPort_caminho = strstr(url_copy, "http://");
    if (hostPort_caminho != NULL) {
        hostPort_caminho += strlen("http://");
    } else {
        hostPort_caminho = url_copy;
    }

    char *caminho_inicial = strchr(hostPort_caminho, '/');
    if (caminho_inicial != NULL) {
        strcpy(path, caminho_inicial);
        *caminho_inicial = '\0';
    } else {
        strcpy(path, "/");
    }

    char *port_inicial = strchr(hostPort_caminho, ':');
    if (port_inicial != NULL) {
        *port_inicial = '\0';
        *port = atoi(port_inicial + 1);
    } else {
        *port = 80;
    }

    strcpy(host, hostPort_caminho);
}

char* obter_nome_arquivo_do_caminho(const char *path) {
    char *path_copy = strdup(path);
    if (path_copy == NULL) {
        perror("Erro ao duplicar string (strdup)");
        exit(EXIT_FAILURE);
    }
    
    char *nome_base = basename(path_copy);

    if (strlen(nome_base) == 0 || strcmp(nome_base, "/") == 0 || strcmp(nome_base, ".") == 0) {
        free(path_copy);
        return strdup("index.html");
    }
    
    char *resultado = strdup(nome_base);
    
    free(path_copy);
    
    if (resultado == NULL) {
        perror("Erro ao duplicar string (strdup)");
        exit(EXIT_FAILURE);
    }

    return resultado;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <URL_completa>\n", argv[0]);
        fprintf(stderr, "Ex: %s http://localhost:5050/imagem.pdf\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char host[256];
    char caminho[1024];
    int porta;
    
    parse_url(argv[1], host, &porta, caminho);

    char *nome_arquivo_salvar = obter_nome_arquivo_do_caminho(caminho); 

    printf("Conectando a %s na porta %d...\n", host, porta);
    printf("Buscando caminho: %s\n", caminho);
    printf("Salvando como: %s\n", nome_arquivo_salvar);

    struct hostent *servidor = gethostbyname(host);
    if (servidor == NULL) {
        fprintf(stderr, "Erro: Host não encontrado '%s'\n", host);
        free(nome_arquivo_salvar);
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        free(nome_arquivo_salvar);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in endereco_servidor;
    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_port = htons(porta);
    memcpy(&endereco_servidor.sin_addr.s_addr, servidor->h_addr_list[0], servidor->h_length);

    if (connect(sockfd, (struct sockaddr *)&endereco_servidor, sizeof(endereco_servidor)) < 0) {
        perror("Erro ao conectar");
        free(nome_arquivo_salvar);
        exit(EXIT_FAILURE);
    }

    char requisicao[BUFFER_SIZE];
    sprintf(requisicao, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", caminho, host);
    
    if (write(sockfd, requisicao, strlen(requisicao)) < 0) {
        perror("Erro ao escrever no socket");
        free(nome_arquivo_salvar);
        exit(EXIT_FAILURE);
    }

    FILE *arquivo_saida = fopen(nome_arquivo_salvar, "wb");
    if (arquivo_saida == NULL) {
        perror("Erro ao criar arquivo de saída");
        close(sockfd);
        free(nome_arquivo_salvar);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_lidos;
    bool cabecalho_processado = false;
    bool sucesso = false;

    while ((bytes_lidos = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_lidos] = '\0';

        if (!cabecalho_processado) {
            if (strncmp(buffer, "HTTP/1.1 200 OK", 15) != 0) {
                fprintf(stderr, "Erro: O servidor retornou um erro (não foi 200 OK):\n");
                char *fim_cabecalho = strstr(buffer, "\r\n\r\n");
                if (fim_cabecalho) *fim_cabecalho = '\0';
                fprintf(stderr, "%s\n", buffer);
                fclose(arquivo_saida);
                remove(nome_arquivo_salvar);
                close(sockfd);
                free(nome_arquivo_salvar);
                return 1;
            }
            
            sucesso = true;

            char *inicio_corpo = strstr(buffer, "\r\n\r\n");
            
            if (inicio_corpo != NULL) {
                inicio_corpo += 4;
                
                size_t tam_corpo = bytes_lidos - (inicio_corpo - buffer);
                
                fwrite(inicio_corpo, 1, tam_corpo, arquivo_saida);
                
                cabecalho_processado = true;
            }
        } else {
            fwrite(buffer, 1, bytes_lidos, arquivo_saida);
        }
    }

    if (bytes_lidos < 0) {
        perror("Erro ao ler do socket");
    }

    if (sucesso) {
        printf("Arquivo salvo com sucesso: %s\n", nome_arquivo_salvar);
    }

    fclose(arquivo_saida);
    close(sockfd);
    free(nome_arquivo_salvar);

    return 0;
}