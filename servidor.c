#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTA 5050
#define BUFFER_SIZE 8192
#define SITE_DIR "./site"  // pasta a servir

// Envia resposta simples
void enviar_resposta(int cliente, const char *tipo_conteudo, const char *conteudo) {
    char cabecalho[256];
    snprintf(cabecalho, sizeof(cabecalho),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Connection: close\r\n\r\n", tipo_conteudo);
    send(cliente, cabecalho, strlen(cabecalho), 0);
    send(cliente, conteudo, strlen(conteudo), 0);
}

// Envia arquivo solicitado
void enviar_arquivo(int cliente, const char *arquivo) {
    char caminho[512];
    snprintf(caminho, sizeof(caminho), "%s/%s", SITE_DIR, arquivo);

    printf("Tentando abrir arquivo: %s\n", caminho); // debug

    FILE *fp = fopen(caminho, "rb");
    if (!fp) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "Connection: close\r\n\r\n"
                 "Erro 404: Arquivo '%s' nao encontrado.\n",
                 arquivo, SITE_DIR);
        send(cliente, msg, strlen(msg), 0);
        return;
    }

    // Detecta tipo de conteúdo
    const char *tipo = "application/octet-stream";
    const char *ext = strrchr(arquivo, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) tipo = "text/html";
        else if (strcmp(ext, ".txt") == 0) tipo = "text/plain";
        else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) tipo = "image/jpeg";
        else if (strcmp(ext, ".png") == 0) tipo = "image/png";
        else if (strcmp(ext, ".gif") == 0) tipo = "image/gif";
    }

    char cabecalho[256];
    snprintf(cabecalho, sizeof(cabecalho),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", tipo);
    send(cliente, cabecalho, strlen(cabecalho), 0);

    char buffer[BUFFER_SIZE];
    size_t lido;
    while ((lido = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        send(cliente, buffer, lido, 0);

    fclose(fp);
}


// Envia lista de arquivos em HTML
void enviar_lista(int cliente) {
    DIR *dir = opendir(SITE_DIR);
    if (!dir) {
        enviar_resposta(cliente, "text/plain", "Erro ao abrir diretório site/.\n");
        return;
    }

    char conteudo[8192];
    strcpy(conteudo,
           "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Servidor HTTP</title>"
           "<style>body{font-family:sans-serif;padding:20px} a{display:block;margin:5px;}</style></head>"
           "<body><h1>Arquivos disponiveis</h1>");

    struct dirent *entrada;
    while ((entrada = readdir(dir)) != NULL) {
        if (entrada->d_type == DT_REG) {  // apenas arquivos
            char linha[256];
            snprintf(linha, sizeof(linha), "<a href=\"/%s\">%s</a>", entrada->d_name, entrada->d_name);
            strcat(conteudo, linha);
        }
    }
    closedir(dir);
    strcat(conteudo, "</body></html>");
    enviar_resposta(cliente, "text/html", conteudo);
}

int main() {
    int servidor, cliente;
    struct sockaddr_in addr_servidor, addr_cliente;
    socklen_t tamanho_cliente;
    char buffer[BUFFER_SIZE];

    servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor < 0) { perror("Erro socket"); return 1; }

    addr_servidor.sin_family = AF_INET;
    addr_servidor.sin_addr.s_addr = INADDR_ANY;
    addr_servidor.sin_port = htons(PORTA);

    if (bind(servidor, (struct sockaddr *)&addr_servidor, sizeof(addr_servidor)) < 0) {
        perror("Erro bind"); close(servidor); return 1;
    }

    listen(servidor, 5);
    printf("Servidor rodando em http://localhost:%d (servindo pasta site/)\n", PORTA);

    while (1) {
        tamanho_cliente = sizeof(addr_cliente);
        cliente = accept(servidor, (struct sockaddr *)&addr_cliente, &tamanho_cliente);
        if (cliente < 0) { perror("Erro accept"); continue; }

        int bytes = recv(cliente, buffer, sizeof(buffer)-1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            char metodo[8], caminho[256];
            sscanf(buffer, "%s %s", metodo, caminho);

            printf("Requisição recebida: %s\n", caminho); // debug

            if (strcmp(caminho, "/") == 0)
                enviar_lista(cliente);
            else {
                char arquivo[512];
                snprintf(arquivo, sizeof(arquivo), "%s", caminho + (caminho[0]=='/'?1:0));
                enviar_arquivo(cliente, arquivo);
            }
        }

        close(cliente);
    }

    close(servidor);
    return 0;
}
