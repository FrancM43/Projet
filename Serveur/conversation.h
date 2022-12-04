#ifndef CONVERSATION_H
#define CONVERSATION_H

#include "server2.h"
#include "historique.h"

typedef struct
{
   char nom[BUF_SIZE];
   Client *participants[MAX_PARTICIPANTS];
   int nb_participants;
   Historique* debut_historique;
   Historique* fin_historique;
}Conversation;

#endif /* guard */