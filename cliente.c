#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 8192

// Função para mostrar lista de arquivos do HTML recebido
void mostrar_lista_html(const char *html) {
    const char *ptr = html;
    printf("Arquivos disponíveis:\n");
    while ((ptr = strstr(ptr, "<a href=\"")) != NULL) {
        ptr += 9; // pula <a href="
        const char *end = strchr(ptr, '"');
        if (!end) break;
        char arquivo[256];
        strncpy(arquivo, ptr, end - ptr);
        arquivo[end - ptr] = '\0';
        printf(" - %s\n", arquivo);
        ptr = end;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <URL>\nExemplo: %s http://localhost:5050/\n", argv[0], argv[0]);
        return 1;
    }

    char *url = argv[1];
    char host[256] = "";
    char path[512] = "";
    int port = 5050;

    if (strncmp(url, "http://", 7) == 0) url += 7;

    if (sscanf(url, "%255[^:/]:%d/%511[^\n]", host, &port, path) != 3) {
        if (sscanf(url, "%255[^:/]/%511[^\n]", host, path) != 2) strcpy(path, "");
        else if (sscanf(url, "%255[^:/]", host) != 1) { fprintf(stderr,"URL inválida\n"); return 1; }
    }

    if (strlen(path) == 0) strcpy(path, "/");

    struct hostent *server = gethostbyname(host);
    if (!server) { fprintf(stderr,"Host não encontrado: %s\n", host); return 1; }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Erro socket"); return 1; }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro conectar"); close(sock); return 1;
    }

    char request[1024];
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
             (path[0]=='/')?path:path, host);

    send(sock, request, strlen(request), 0);

    char buffer[BUFFER_SIZE];
    int bytes;
    int header_done = 0;
    FILE *file = NULL;
    char filename[256];
    char *html_buffer = NULL;
    size_t html_size = 0;

    if (strcmp(path, "/") != 0) {
        const char *slash = strrchr(path, '/');
        strcpy(filename, (slash && *(slash+1)) ? slash+1 : path);
    }

    while ((bytes = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0';

        if (!header_done) {
            char *body = strstr(buffer, "\r\n\r\n");
            if (!body) continue;
            header_done = 1;
            body += 4;

            // Analisa status HTTP
            char status[64];
            sscanf(buffer, "%63s %63[^\r\n]", status, status+4);

            if (strstr(status, "404 Not Found")) {
                printf("%s\n", body);
                close(sock);
                return 1;
            }

            if (strcmp(path,"/")==0) {
                html_buffer = malloc(bytes + 1);
                strcpy(html_buffer, body);
                html_size = bytes - (body-buffer);
            } else {
                file = fopen(filename,"wb");
                if (!file){ perror("Erro criar arquivo"); close(sock); return 1;}
                fwrite(body, 1, bytes - (body-buffer), file);
            }
        } else {
            if (file) fwrite(buffer,1,bytes,file);
            else if (strcmp(path,"/")==0) {
                html_buffer = realloc(html_buffer, html_size + bytes + 1);
                memcpy(html_buffer + html_size, buffer, bytes);
                html_size += bytes;
                html_buffer[html_size] = '\0';
            }
        }
    }

    if(file) { fclose(file); printf("Arquivo salvo: %s\n", filename); }
    else if(html_buffer) { mostrar_lista_html(html_buffer); free(html_buffer); }

    close(sock);
    return 0;
}
