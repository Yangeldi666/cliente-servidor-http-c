CC = gcc
CFLAGS = -Wall

CLIENTE = cliente
SERVIDOR = servidor

all: $(CLIENTE) $(SERVIDOR)

$(CLIENTE): cliente.c
	$(CC) $(CFLAGS) cliente.c -o $(CLIENTE)

$(SERVIDOR): servidor.c
	$(CC) $(CFLAGS) servidor.c -o $(SERVIDOR)

clean:
	rm -f $(CLIENTE) $(SERVIDOR)
