#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 8192

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <URL>\nExemplo: %s http://localhost:5050/arquivo.txt\n", argv[0], argv[0]);
        return 1;
    }

    char *url = argv[1];
    char host[256], path[256];
    int port = 80;

    // Parse da URL
    if (sscanf(url, "http://%255[^:/]:%d/%255[^\n]", host, &port, path) < 2) {
        if (sscanf(url, "http://%255[^/]/%255[^\n]", host, path) < 1) {
            fprintf(stderr, "URL inválida\n");
            return 1;
        }
    }

    // Resolver host
    struct hostent *server = gethostbyname(host);
    if (!server) {
        fprintf(stderr, "Erro ao resolver host\n");
        return 1;
    }

    // Criar socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("Erro ao criar socket"); return 1; }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro ao conectar");
        close(sockfd);
        return 1;
    }

    // Construir requisição HTTP segura
    char request[512];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);

    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("Erro ao enviar requisição");
        close(sockfd);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    int bytes_received;
    int header_done = 0;
    char status_line[256];
    char filename[256];
    strcpy(filename, strrchr(path, '/') ? strrchr(path, '/') + 1 : path);
    FILE *file = NULL;

    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        if (!header_done) {
            char *body_start = strstr(buffer, "\r\n\r\n");
            if (body_start) {
                header_done = 1;
                sscanf(buffer, "%255[^\r\n]", status_line);

                if (strstr(status_line, "200 OK")) {
                    // Só criar arquivo se status for 200
                    file = fopen(filename, "wb");
                    if (!file) { perror("Erro ao abrir arquivo"); close(sockfd); return 1; }
                    fwrite(body_start + 4, 1, bytes_received - (body_start - buffer + 4), file);
                } else {
                    // Arquivo não encontrado: imprimir mensagem/texto do servidor
                    printf("%s", body_start + 4);
                    file = NULL;
                }
            }
        } else {
            if (file) {
                fwrite(buffer, 1, bytes_received, file);
            } else {
                printf("%s", buffer);
            }
        }
    }

    if (file) {
        fclose(file);
        printf("Arquivo salvo como: %s\n", filename);
    }

    close(sockfd);
    return 0;
}
