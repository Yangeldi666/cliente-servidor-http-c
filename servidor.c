#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 5050
#define BUFFER_SIZE 8192
#define DIR_PATH "./site"  // Diretório a servir

// Envia resposta HTTP simples
void send_response(int client_sock, const char *status, const char *content_type, const char *body) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.0 %s\r\nContent-Type: %s\r\n\r\n%s", status, content_type, body);
    send(client_sock, response, strlen(response), 0);
}

// Servir arquivo solicitado
void serve_file(int client_sock, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        // Arquivo não encontrado: listar arquivos em texto puro
        DIR *dir = opendir(DIR_PATH);
        char body[BUFFER_SIZE] = "Arquivo não encontrado.\nArquivos disponíveis:\n";
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    strncat(body, entry->d_name, sizeof(body) - strlen(body) - 2);
                    strncat(body, "\n", sizeof(body) - strlen(body) - 1);
                }
            }
            closedir(dir);
        }
        send_response(client_sock, "404 Not Found", "text/plain", body);
        return;
    }

    // Arquivo encontrado: enviar conteúdo
    char header[256];
    snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n");
    send(client_sock, header, strlen(header), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_sock, buffer, bytes_read, 0);
    }
    fclose(file);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("Erro ao criar socket"); return 1; }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro no bind"); close(server_sock); return 1;
    }

    listen(server_sock, 5);
    printf("Servidor rodando na porta %d, servindo diretório: %s\n", PORT, DIR_PATH);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) { perror("Erro no accept"); continue; }

        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            char method[16], path[256];

            if (sscanf(buffer, "%15s %255s", method, path) == 2 && strcmp(method, "GET") == 0) {
                char filepath[512];
                snprintf(filepath, sizeof(filepath), "%s%s", DIR_PATH, path);
                serve_file(client_sock, filepath);
            } else {
                send_response(client_sock, "400 Bad Request", "text/plain", "Requisição inválida\n");
            }
        }

        close(client_sock);
    }

    close(server_sock);
    return 0;
}
