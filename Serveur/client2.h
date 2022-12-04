#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"
#include "conversation.h"

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char password[BUF_SIZE];
   Conversation *conversations[MAX_CONVERSATIONS];
   Conversation *conversation_actuelle;
   int actual_conversation;
   int estConnecte;
};

#endif /* guard */
