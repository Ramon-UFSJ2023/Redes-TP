#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define PATH_MAX 1024

void handle_connection(int client_socket, const char *base_dir);
void send_response(int socket, const char *status, const char *content_type, const char *body, long content_length);
void send_file(int socket, const char *filepath);
void send_directory_listing(int socket, const char *dirpath);
void send_not_found(int socket);
const char *get_content_type(const char *filename);


int main(int argc, char *argv[]){
    int servidor_fd, novo_socket;
    struct sockaddr_in addres;
    int addrlen = sizeof(addres);

    if(argc != 2){exit(EXIT_FAILURE);}
    const char *base_dir = argv[1];

    if((servidor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Erro ao criar o socket");
        exit(EXIT_FAILURE);
    }

    addres.sin_family = AF_INET;
    addres.sin_addr.s_addr = INADDR_ANY;
    addres.sin_port = htons(PORT);

    if(bind(servidor_fd,(struct sockaddr *)&addres, sizeof(addres))< 0){
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }

    if(listen(servidor_fd, 3)<0){
        perror("Erro na escuta");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escutando na porta %d\n", PORT);
    printf("Diretorio: %s\n", base_dir);
    while(1){
        if((novo_socket = accept(servidor_fd, (struct sockaddr *)&addres, (socklen_t*)&addrlen))<0){
            perror("Erro no accept");
            continue;
        }
        handle_connection(novo_socket, base_dir);
        close(novo_socket);
    }
    return 0;
}



void handle_connection(int cliente_socket, const char *base_dir){
    char buffer[BUFFER_SIZE] = {0};
    char method[16], path[PATH_MAX], protocol[16];

    if(read(cliente_socket, buffer, BUFFER_SIZE-1)<0){ perror("Erro na leitura do scoket"); return;}
    sscanf(buffer, "%s %s %s", method, path, protocol);
    printf("Requisição: %s %s\n", method, path);

    if(strcmp(method, "GET") != 0){
        send_response(cliente_socket, "501 Not Implementend", "text/html", "<html><body><h1>501 Not Implemented</h1></body></html>", -1);
        return;
    }

    char path_completo[PATH_MAX];
    if (strcmp(path, "/") == 0) {
        sprintf(path_completo, "%s", base_dir);
    } else {
        sprintf(path_completo, "%s%s", base_dir, path + 1); 
    }

    if(strstr(path, "..") != NULL){
        send_not_found(cliente_socket);
        return;
    }

    struct stat path_stat;

    if(stat(path_completo, &path_stat) !=0){
        send_not_found(cliente_socket);
        return;
    }

    if(S_ISDIR(path_stat.st_mode)){
        if(path[strlen(path)-1]!= '/'){
            strcat(path_completo, "/");
        }

        char index_caminho[PATH_MAX];
        sprintf(index_caminho, "%sindex.html", path_completo);

        if(access(index_caminho, F_OK) == 0){
            send_file(cliente_socket, index_caminho);
        }else{send_directory_listing(cliente_socket, path_completo);}
    }
    else if(S_ISREG(path_stat.st_mode)){
        send_file(cliente_socket, path_completo);
    }else{ send_not_found(cliente_socket);}
    
}

void send_response(int socket, const char *status, const char *content_type, const char *body, long content_length){
    char header[BUFFER_SIZE];

    if(content_length<0){content_length =(body == NULL)? 0 : strlen(body);}

    sprintf(header, "HTTP/1.1 %s\r\n", status);
    sprintf(header + strlen(header), "Content-Type: %s\r\n", content_type);
    sprintf(header + strlen(header), "Content-Length: %ld\r\n", content_length);
    sprintf(header + strlen(header), "Connection: close\r\n");
    sprintf(header + strlen(header), "\r\n");

    write(socket, header, strlen(header));

    if(body != NULL){ write(socket, body, content_length);}
}

void send_file(int socket, const char *path_arq){
    int fd = open(path_arq, O_RDONLY);
    if(fd <0){
        perror("Erro ao abrir arquivo");
        send_not_found(socket);
        return;
    }

    struct stat file_stat;
    fstat(fd, &file_stat);
    long file_tamanho = file_stat.st_size;

    const char *content_type = get_content_type(path_arq);
    send_response(socket, "200 OK", content_type, NULL, file_tamanho);

    char file_buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while((bytes_read = read(fd, file_buffer, BUFFER_SIZE))>0){
        if(write(socket, file_buffer, bytes_read) != bytes_read){
            perror("Erro ao escrever no socket");
            break;
        }
    }
    close(fd);
}

void send_directory_listing(int socket, const char *dirpath){
    char html_body[BUFFER_SIZE*4]={0};
    char *ptr = html_body;

    ptr += sprintf(ptr, "<html><head><title>Index of %s</title></head>", dirpath);
    ptr += sprintf(ptr, "<body><h1>Index of %s</h1><ul>", dirpath);

    DIR *d = opendir(dirpath);
    if(d == NULL){
        perror("Erro ao abrir diretorio");
        send_not_found(socket);
        return;
    }
    struct dirent *dir_entry;

    while((dir_entry = readdir(d))!= NULL){
        if(dir_entry->d_name[0]=='.'){continue;}

        ptr += sprintf(ptr, "<li><a href=\"%s\">%s</a></li>", dir_entry->d_name, dir_entry->d_name);
    }
    closedir(d);
    ptr += sprintf(ptr, "</ul></body></html>");
    send_response(socket, "200 OK", "text/html", html_body, -1);
}

void send_not_found(int socket) {
    const char *body = "<html><body><h1>404 Not Found</h1><p>Hoje nao foi seu dia, tente amanha ;).</p></body></html>";
    send_response(socket, "404 Not Found", "text/html", body, -1);
}

const char *get_content_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream"; // Binário genérico

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    
    return "application/octet-stream";
}