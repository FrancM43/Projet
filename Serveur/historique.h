#ifndef HISTORIQUE_H
#define HISTORIQUE_H

#include "server2.h"

typedef struct Historique Historique;

struct Historique{
	char message[BUF_SIZE];
    char expediteur[BUF_SIZE];
	Historique *message_suivant;
	Historique *message_precedent;
};

#endif /* guard */