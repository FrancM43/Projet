#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_PARTICIPANTS 100
#define MAX_CONVERSATIONS 50

#define BUF_SIZE    1024

typedef struct Client Client;
#include "conversation.h"
#include "historique.h"
#include "client2.h"

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void send_message_to_conversation(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_client(Client *client, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
Client * recupererClientParNom(Client *clients, int nb_clients, const char *nomClient);
static Conversation *creer_conversation(Client *client, char *nom_conversation);
static void inviter_conversation(Client *clients, int actual, Conversation *conversation, char *participant);
static void afficher_historique_conversation(Client *client);
static void persister_client(Client *client);
static void persister_conversation(Conversation *conversation);
static void persister_participants(const char* nom_conversation, const char *participant);
static void persister_historique(const char* nom_conversation, Historique *historique);
static int charger_clients(Client *clients, const char* fichier_clients);
static int charger_conversations(Conversation *conversations, const char *fichier_conversations);
void charger_participants(Client *clients, int nb_clients, Conversation *conversation);
void charger_historique(Conversation *conversation);
static void afficher_client(Client *client);
static void afficher_historique(Conversation *conversation);
static void afficher_clients(Client *clients, int nbClients);
const char* findPwd(char* name);
int est_dans_conversation(Client *client, char *param);
int findConversation(Client *clients, char *param, int i);

#endif /* guard */
