# Trabalho HTTP em C - Cliente e Servidor

Este projeto contém um **cliente HTTP** e um **servidor HTTP** simples escritos em C.

## Funcionalidades

### Servidor
- Serve arquivos de um diretório especificado.
- Porta padrão: 5050.
- Retorna 404 se o arquivo não existir.

### Cliente
- Baixa arquivos via HTTP (HTML, JPG, PDF, etc.).
- Salva com o mesmo nome do arquivo solicitado.

## Como usar

### Compilar
make

### Rodar servidor
./servidor site

### Rodar Cliente
./cliente http://localhost:5050/index.html
./cliente http://localhost:5050/imagem.jpg
./cliente http://localhost:5050/arquivo.pdf

### Limpar executáveis
make clean