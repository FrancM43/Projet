# http://www.lab.dit.upm.es/~cdatlab/exs/rpc/test/
# rpcgen example Makefile
# Internetworking with TCP/IP, Volume III
#
CC=gcc
PROGS = client server

DEFS = 
INCLUDE =
CFLAGS = -g ${DEFS} ${INCLUDE}

SERVER_DEPS = Serveur/client2.h Serveur/conversation.h Serveur/historique.h Serveur/server2.h
CLIENT_DEPS = Clients/client2.h

SRC = server2.c client2.c
GFILES =

all: ${PROGS}

client : Clients/client2.c
	${CC} ${CFLAGS} -lnsl -o $@ $^

server : Serveur/server2.c
	${CC} ${CFLAGS} -lnsl -o $@ $^

clean:
	rm server
	rm client
	rm *.txt


