#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   char param[BUF_SIZE];
   char message[BUF_SIZE];
   /* the index for the array */
   int actual; //le nombre de clients chargés
   int nb_clients_connectes = 0;
   int nb_conversations = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   /* Un tableau pour toutes les conversations */
   Conversation conversations[MAX_CONVERSATIONS];
   int i;

   fd_set rdfs;

   /* Chargement de l'ensemble des données de l'application*/
   nb_conversations = charger_conversations(conversations, "conversations.txt");
   actual = charger_clients(clients, "clients.txt");
   for(i=0; i<nb_conversations; i++)
   {
      charger_participants(clients, actual, &conversations[i]);
      charger_historique(&conversations[i]);
   }
   
   printf("Nombre de clients chargés : %d \n", actual);

   while(1)
   {
      i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         if(clients[i].estConnecte==1)
         {
            FD_SET(clients[i].sock, &rdfs);
         }
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }
         printf("Le client \" %s \" tente de se connecter \r\n", buffer);

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         i=0;
         /* 
          * Gestion de la connection d'un client, on vérifie s'il existe.
          * Le cas échéant, son mot de passe est vérifié
          * Sinon, l'utilisateur entre un mot de passe
         */
         Client *clientExistant;
         clientExistant = recupererClientParNom(clients, actual, buffer);
         if(clientExistant!=NULL)
         {
            write_client(csock, "-----------------> Veuillez entrer votre mot de passe :");
            read_client(csock, buffer);
            if(strcmp(clientExistant->password,buffer)!=0)
            {
               write_client(csock, "/!\\/!\\/!\\ Mot de passe incorrect ! \nDéconnexion...");
               clientExistant->estConnecte = 0;
               closesocket(csock);
            } else{
               clients[i].sock = csock;
               write_client(clientExistant->sock, "----------------- Connexion réussie ! -----------------");
               clientExistant->estConnecte = 1;
               nb_clients_connectes++;
            }
         }else{
            Client c = { csock };
            strncpy(c.name, buffer, BUF_SIZE - 1);
            c.actual_conversation = 0;
            c.conversation_actuelle = NULL;
            write_client(c.sock, "-----------------> Veuillez entrer un mot de passe :");
            read_client(c.sock, buffer);
            strncpy(c.password, buffer, BUF_SIZE-1);
            c.estConnecte = 1;
            clients[actual++] = c;
            persister_client(clients+actual-1);     //On utilise le pointeur pour incrémenter du bon nombre de client
            write_client(c.sock, "----------------- Mot de passe enregistré. -----------------\n----------------- Connexion réussie ! -----------------");

            printf("Le client vient de se connecter avec succes !\r\n");
            nb_clients_connectes++;
         }

         printf("Nombre de client(s) actuellement connecté(s) : %d \r\n\n", nb_clients_connectes);
      }
      else
      {
         int i = 0;
         printf("Nombre de personnes dans clients : %d \n", actual);
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            printf("Oui %d\r\n",clients[i].estConnecte);
            if(clients[i].estConnecte==1 && FD_ISSET(clients[i].sock, &rdfs))
            {
               Client *client = &clients[i];
               int c = read_client(client->sock, buffer);

               //On écoute les commandes, celles-ci commencent par un '/'
               if (buffer[0] == '/')
               {
                  char *commande = strtok(buffer, " ");
                  char *param = strtok(NULL, " ");
                  //strncpy(param,param_in,strlen(param_in)+1);
                  //param = strncat(NULL, strtok(NULL, " "),BUF_SIZE - 1);
                  if (strcmp(commande, "/quit") == 0)
                  {
                     write_client(clients[i].sock, "----------------- Déconnexion... -----------------");
                     closesocket(clients[i].sock);
                     remove_client(clients, i, &actual);
                     printf("Client %s disconnected from server !\n", client[i].name);
                     strncpy(buffer, client->name, BUF_SIZE - 1);
                     strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  }
                  else if (strcmp(commande, "/group") == 0)
                  {
                     if (param == NULL)
                     {
                        printf("Client %s : Aucun nom de groupe renseigné !\n", client[i].name);
                        write_client(clients[i].sock, "\n/!\\/!\\/!\\ Aucun nom de groupe renseigné ! /!\\/!\\/!\\");
                     }
                     else
                     {
                        int j = findConversation(clients, param, i);
                        if(j==-1){
                           creer_conversation(client, param);
                           printf("Client %s vient de créer le groupe %s !\n", client[i].name, param);
                           sprintf(message, "\n----------------- Vous venez de créer le groupe %s ! -----------------\n", param);
                           write_client(clients[i].sock, message);
                        }else{
                           printf("Client %s : Le groupe %s existe déjà !\n", client[i].name, param);
                           sprintf(message, "\n/!\\/!\\/!\\ Le groupe %s existe déjà ! /!\\/!\\/!\\\n", param);
                           write_client(clients[i].sock, message);
                        }
                        //afficher_client(client);
                     }
                  }
                  else if (strcmp(commande, "/join") == 0)
                  {
                     if (param == NULL)
                     {
                        printf("Client %s : Aucun nom de groupe renseigné !\n", client[i].name);
                        write_client(clients[i].sock, "\n/!\\/!\\/!\\ Aucun nom de groupe renseigné ! /!\\/!\\/!\\");
                     }
                     else
                     {
                        printf("-----------Début de join, le nom de la conversation actuelle du client %s est %s\n",client->name, client->conversation_actuelle->nom);
                        i=0;
                        while(i < client->actual_conversation && strcmp(client->conversations[i++]->nom,param)!= 0)
                        {
                           //On entre jamais dans cette boucle !
                           printf("Conversation %d : %s     Objectif : %s     Resultat : %d\r\n", i-1, client->conversations[i-1]->nom, param, strcmp(client->conversations[i-1]->nom,param));
                        }
                        if(--i >= 0 && strcmp(client->conversations[i]->nom,param) == 0)
                        {
                           sprintf(message, "\n----------------- Vous avez bien rejoint le groupe %s ! -----------------\n----------------- Historique de la conversation : \n", param);
                           write_client(client->sock, message);
                           client->conversation_actuelle = client->conversations[i];
                           afficher_historique_conversation(client);
                           printf("Le client %s vient de rejoindre la conversation %s\r\n", client->name, client->conversation_actuelle->nom);
                           //persister_participants(client->conversation_actuelle, client->name);
                        }else {
                           sprintf(message, "\n/!\\/!\\/!\\ Vous n'avez aucun groupe de ce nom : %s ! /!\\/!\\/!\\\n", param);
                           write_client(client->sock, message);

                        }
                     }
                  }
                  else if (strcmp(commande, "/invite") == 0)
                  {
                     if (param == NULL)
                     {
                        printf("Client %s : aucun nom de client renseigné !\n", client[i].name);
                        write_client(clients[i].sock, "\n/!\\/!\\/!\\ Il faut renseigner une personne à inviter ! /!\\/!\\/!\\");
                     }
                     else
                     {
                        //on vérifie si le client n'est pas déjà dans la conversation
                        if(est_dans_conversation(client, param)==1){
                           printf("Client %s : %s est déjà dans la conversation !\n", client[i].name, param);
                           sprintf(message, "\n/!\\/!\\/!\\ %s est déjà dans la conversation ! /!\\/!\\/!\\\n", param);
                           write_client(clients[i].sock, message);
                        }else{
                           if(client->conversation_actuelle != NULL){
                              inviter_conversation(clients, actual, client->conversation_actuelle, param);
                              printf("Client  : %s a invité le client : %s dans le groupe : %s\n", client[i].name, param, client->conversation_actuelle->nom);
                              sprintf(message, "\n----------------- Vous avez bien ajouter %s au groupe %s ! -----------------\\n", param, client->conversation_actuelle->nom);
                              write_client(client->sock, message);
                              //write_client(client->sock, "La personne a bien été invité dans le groupe !");
                           }else{
                              write_client(client->sock, "\n/!\\/!\\/!\\ Vous devez d'abord vous connecter à une conversation ! /!\\/!\\/!\\");
                           }
                        }
                     }
                  }
                  else if (strcmp(commande, "/infos") == 0)
                  {
                     if (param != NULL)
                     {
                        printf("La commande \"infos\" ne prend pas de paramètre : %s!\n", param);
                        sprintf(message, "/!\\/!\\/!\\ La commande \"infos\" ne prend pas de paramètre : %s! /!\\/!\\/!\\\n", param);
                        write_client(client->sock, message);
                     }
                     else
                     {
                        sprintf(message, "\n----------------- Nombre de personnes actuellement connectés : %d -----------------\n",nb_clients_connectes);
                        write_client(client->sock, message);
                     }
                  }
                  else if (strcmp(commande, "/help") == 0)
                  {
                     if (param != NULL)
                     {
                        printf("La commande \"help\" ne prend pas de paramètre !\n");
                        sprintf(message, "/!\\/!\\/!\\ La commande \"help\" ne prend pas de paramètre : %s! /!\\/!\\/!\\\n", param);
                        write_client(client->sock, message);
                     }
                     else
                     {
                        write_client(client->sock, "\n----------------- Manuel d'utlisation : \n - Pour créer un groupe : /group nom_groupe \n - Pour inviter une personne dans le groupe : /invite nom_personne \n - Pour rejoindre un groupe : /join nom_groupe \n - Pour afficher les informations du groupe : /infos \nFin de l'aide ! -----------------\n");
                     }
                  }
                  else
                  {
                     printf("Client %s : command inconnue !\n", client[i].name);
                     write_client(client->sock, "/!\\/!\\/!\\ Commande inconnue ! /!\\/!\\/!\\ \n----------------- Tapez /help pour afficher le manuel d'utilisation ! -----------------\n");
                  }
               }else
               {
                  /* client disconnected */
                  if(c == 0)
                  {
                     client->estConnecte = 0;
                     closesocket(clients[i].sock);
                     nb_clients_connectes--;
                     strncpy(buffer, client->name, BUF_SIZE - 1);
                     strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                     nb_clients_connectes--;
                     if(client->conversation_actuelle!=NULL)
                     {
                        send_message_to_conversation(clients, *client, actual, buffer, 1);
                     }
                     //send_message_to_all_clients(clients, *client, actual, buffer, 1);
                  }
                  else
                  {
                     //send_message_to_all_clients(clients, *client, actual, buffer, 0);
                     if(client->conversation_actuelle!=NULL)
                     {
                        //debug
                        // printf("nb de clients : %d\n", actual);
                        // printf("le nom du client : %s\n", client->name);
                        send_message_to_conversation(clients, *client, actual, buffer, 0);
                     }
                  }
                  break;
               }
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

int findConversation(Client *clients, char *param, int i){
   int result = -1;
   //On vérifie que la conversation n'existe pas déjà, dans le fichier conversations.txt
   FILE* fichier = NULL;
   char ligne[BUF_SIZE];
   char nom_conversation_fichier[BUF_SIZE];
   fichier = fopen("conversations.txt", "r");
   if(fichier != NULL)
   {
      while(fgets(ligne, BUF_SIZE, fichier) != NULL)
      {
         sscanf(ligne, "%s", nom_conversation_fichier);
         if(strcmp(nom_conversation_fichier, param) == 0)
         {
            result = 1;
         }
      }
      fclose(fichier);
   }
   return result;
}

Client * recupererClientParNom(Client *clients, int nb_clients, const char *nomClient)
{
   int i = 0;
   while(i < nb_clients && strcmp(clients[i++].name,nomClient)!=0);
   if(strcmp(clients[--i].name,nomClient) == 0)
   {
      return &clients[i];
   }
   return NULL;
}

//Fonction findPwd : Trouve le mot de passe d'un client en fonction de son nom d'utilisateur (fonctionne avec le fichier users.txt)
//Retourne le mot de passe du client en const char*
const char* findPwd(char* name)
{
   FILE* fichier = NULL;
   char ligne[100] = "";
   char* nom = NULL;
   char* pwd = NULL;
   char* token = NULL;
   const char* delim = " ";
   fichier = fopen("users.txt", "r");
   if (fichier != NULL)
   {
      while (fgets(ligne, 100, fichier) != NULL)
      {
         token = strtok(ligne, delim);
         nom = token;
         token = strtok(NULL, delim);
         pwd = token;
         if (strcmp(nom, name) == 0)
         {
            return pwd;
         }
      }
      fclose(fichier);
   }
   return NULL;
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *client, int to_remove, int *actual)
{
   /* number client - 1 */
   (*actual)--;
   printf("Nombre de client(s) actuellement connecté(s) : %d \r\n\n", *actual);
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static void send_message_to_conversation(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;

   //debug
   // printf("le nom du client : %s\n", sender.name);
   // printf("contenue du buffer : %s\n", buffer);
   // printf("actual : %d\n", actual);
   // printf("from_server : %d\n", from_server);

   //Ajout du message à l'historique
   Historique* nouveau_historique = (Historique*) malloc(sizeof(Historique));
   strncpy(nouveau_historique->expediteur, sender.name, BUF_SIZE-1);

   //debug 
   //printf("le nom de l'expediteur : %s\n", nouveau_historique->expediteur);

   strncpy(nouveau_historique->message, buffer, BUF_SIZE - 1);
   nouveau_historique->message_suivant = NULL;
   if(sender.conversation_actuelle->debut_historique == NULL)
   {
      sender.conversation_actuelle->debut_historique = nouveau_historique;
      sender.conversation_actuelle->fin_historique = nouveau_historique;
   } else{
      sender.conversation_actuelle->fin_historique->message_suivant = nouveau_historique;
      nouveau_historique->message_precedent = sender.conversation_actuelle->fin_historique;
      sender.conversation_actuelle->fin_historique = nouveau_historique;
   }
   afficher_historique(sender.conversation_actuelle);

   //debug
   // printf("le nom de la conversation est : %s\n", sender.conversation_actuelle->nom);
   // printf("actual = %d\n", actual);
   // printf("buffer = %s\n", buffer);
   // printf("from_server = %d\n", from_server);
   // printf("historique : %s\n", nouveau_historique->expediteur);

   persister_historique(sender.conversation_actuelle->nom, nouveau_historique);
   
   for(i = 0; i < actual; i++)
   {
      //debug 
      // printf("la socket du client : %d\n", clients[i].sock);
      // printf("la socket du sender : %d\n", sender.sock);
      // printf("nombre de client : %d\n", actual);

      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock && sender.conversation_actuelle == clients[i].conversation_actuelle && clients[i].sock != 0)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{

   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static Conversation *creer_conversation(Client *client, char *nom_conversation)
{
   Conversation* conversation = (Conversation*) malloc(sizeof(Conversation));
   conversation->debut_historique = NULL;
   conversation->fin_historique = NULL;
   conversation->nb_participants = 0;
   conversation->participants[conversation->nb_participants++] = client;
   strncpy(conversation->nom, nom_conversation, BUF_SIZE - 1);
   client->conversations[client->actual_conversation++] = conversation;
   client->conversation_actuelle = conversation;

   //Persistence
   persister_conversation(conversation);
   persister_participants(conversation->nom, client->name);

   //afficher_client(client);
   return conversation;
}

//Vérifier que param n'est pas déjà dans la conversation client->conversation_actuelle 
//retourne 1 si le paramètre est déjà dans la conversation
int est_dans_conversation(Client *client, char *param)
{
   int i = 0;
   for(i = 0; i < client->conversation_actuelle->nb_participants; i++)
   {
      if(strcmp(client->conversation_actuelle->participants[i]->name, param) == 0)
      {
         return 1;
      }
   }
   return 0;
}

static void inviter_conversation(Client *clients, int actual, Conversation *conversation, char *participant)
{
   int i = 0;
   while(i < actual && strcmp(clients[i++].name,participant));
   if(strcmp(clients[--i].name,participant) == 0)
   {
      clients[i].conversations[clients[i].actual_conversation++] = conversation;        //Rajouter une erreur quand on arrive à 50
      conversation->participants[conversation->nb_participants++] = &clients[i];
      persister_participants(conversation->nom, participant);
      printf("Le client %s vient d'etre ajoute dans %s\r\n", participant, conversation->nom);
   }

}

//Pour l'instant on estime que le serveur n'envoie pas de messages à un groupe
static void afficher_historique_conversation(Client *client)
{
   afficher_historique(client->conversation_actuelle);
   Historique* message = client->conversation_actuelle->debut_historique;
   char buffer[BUF_SIZE];
   while(message != NULL)
   {
      if(strcmp(client->name, message->expediteur)!=0)
      {
         strncpy(buffer, message->expediteur, BUF_SIZE - 1);
      } else{
         strncpy(buffer, "                         moi", BUF_SIZE - 1);
      }
      strncat(buffer, " : ", sizeof buffer - strlen(buffer) - 1);
      strncat(buffer, message->message, sizeof buffer - strlen(buffer) - 1);
      strncat(buffer, "\n", sizeof buffer - strlen(buffer) - 1);
      write_client(client->sock, buffer);
      message = message->message_suivant;
   }
}

static int charger_clients(Client *clients, const char *fichier_clients)
{
   int nbClients = 0;

   char buffer[BUF_SIZE];
   char *ptr;

   FILE* fichier = fopen(fichier_clients, "r");
   if(fichier!=0)
   {
      fgets(buffer, BUF_SIZE-1, fichier);
      while(!feof(fichier))
      {
         clients[nbClients].actual_conversation = 0;
         clients[nbClients].conversation_actuelle = NULL;
         clients[nbClients].estConnecte = 0;
         ptr = strtok(buffer, ";");
         strncpy(clients[nbClients].name, ptr, BUF_SIZE-1);
         ptr = strtok(NULL, "\n");
         strncpy(clients[nbClients].password, ptr, BUF_SIZE-1);

         //Initialisation du socket
         clients[nbClients].sock = 0;
         //printf("oui\r\n");
         nbClients++;

         fgets(buffer, BUF_SIZE-1, fichier);
      }
      fclose(fichier);
   }

   return nbClients;
}

//à partir du fichier new_historique.txt, on charge les messages dans la conversation new
void charger_historique(Conversation *conversation)
{
   // //trouver la conversation avec le nom nom_conversation
   // Conversation* new = NULL;
   // int i=0;
   // while(i < nb_conv && strcmp(clients[num_client].conversations[i++]->nom ,nom_conversation));
   // if(strcmp(clients[num_client].conversations[--i]->nom,nom_conversation) == 0)
   // {
   //    new = clients[num_client].conversations[i];
   // }
   conversation->debut_historique = NULL;
   conversation->fin_historique = NULL;

   char buffer[BUF_SIZE];

   strncpy(buffer, conversation->nom, BUF_SIZE - 1);
   strncat(buffer, "_historique.txt", sizeof buffer - strlen(buffer) - 1);

   char *ptr;
   Historique* nouveau_historique;
   FILE* fichier = fopen(buffer, "r");
   if(fichier!=0)
   {
      fgets(buffer, BUF_SIZE-1, fichier);
      while(!feof(fichier))
      {
         nouveau_historique = (Historique*) malloc(sizeof(Historique));
         nouveau_historique->message_precedent = NULL;
         nouveau_historique->message_suivant = NULL;
         ptr = strtok(buffer, ";");
         //Ajout du message à l'historique
         strncpy(nouveau_historique->expediteur, ptr, BUF_SIZE-1);
         ptr = strtok(NULL, "\n");
         strncpy(nouveau_historique->message, ptr, BUF_SIZE - 1);

         if(conversation->fin_historique == NULL)
         {
            conversation->debut_historique = nouveau_historique;
            conversation->fin_historique = nouveau_historique;
         } else{
            conversation->fin_historique->message_suivant = nouveau_historique;
            nouveau_historique->message_precedent = conversation->fin_historique;
            conversation->fin_historique = nouveau_historique;
         }
         fgets(buffer, BUF_SIZE-1, fichier);
      }
      fclose(fichier);
   }
}

//On charge tous les participants de chaque conversation
//De plus, la liste des conversations des clients est mise à jour en conséquence
void charger_participants(Client *clients, int nb_clients, Conversation *conversation)
{
   conversation->nb_participants = 0;

   //On construit le nom de fichier de l'historique
   char nom_fichier_participants[BUF_SIZE];
   strncpy(nom_fichier_participants, conversation->nom, BUF_SIZE - 1);
   strncat(nom_fichier_participants, "_participants.txt", sizeof nom_fichier_participants - strlen(nom_fichier_participants) - 1);

   char buffer[BUF_SIZE];
   char *ptr;

   FILE* fichier = fopen(nom_fichier_participants, "r");

   Client *client;
   if(fichier!=0)
   {
      fgets(buffer, BUF_SIZE-1, fichier);
      while(!feof(fichier))
      {
         buffer[strcspn(buffer, "\n")] = 0;
         client = recupererClientParNom(clients, nb_clients, buffer);
         if(client!=NULL)
         {
            conversation->participants[conversation->nb_participants++] = client;
            client->conversations[client->actual_conversation++] = conversation;
         }
         fgets(buffer, BUF_SIZE-1, fichier);
      }
      fclose(fichier);
   }
   //afficher le nombre de participants dans new
   printf("Il y a %d participants dans %s\r\n", conversation->nb_participants, conversation->nom);
}


//Charge les conversations (seulement le nom) et renvoie le nombre de conversations ainsi créées
static int charger_conversations(Conversation *conversations, const char *fichier_conversations)
{
   int nb_conversations = 0;
   char buffer[BUF_SIZE];

   FILE *fichier = fopen(fichier_conversations, "r");
   if(fichier!=0)
   {
      fgets(buffer, BUF_SIZE-1, fichier);
      while(!feof(fichier))
      {
         buffer[strcspn(buffer, "\n")] = 0;     //Suppression du newline dans le buffer
         strncpy(conversations[nb_conversations++].nom, buffer, BUF_SIZE-1);

         fgets(buffer, BUF_SIZE-1, fichier);
      }
   }

   return nb_conversations;
}

static void persister_client(Client *client)
{
   FILE* fichier = fopen("clients.txt", "a");

   //Ajout du nom du client et son mot de passe dans le fichier
   char buffer[BUF_SIZE];
   strncpy(buffer, client->name, BUF_SIZE-1);
   strncat(buffer, ";", BUF_SIZE-1);
   strncat(buffer, client->password, BUF_SIZE-1);
   strncat(buffer, "\n", BUF_SIZE-1);
   fputs(buffer, fichier);

   fclose(fichier);
}

static void persister_conversation(Conversation *conversation)
{
   FILE* fichier = fopen("conversations.txt", "a");

   //Ajout du nom de la conversation dans le fichier
   char buffer[BUF_SIZE];
   strncpy(buffer, conversation->nom, BUF_SIZE-1);
   strncat(buffer, "\n", BUF_SIZE-1);
   fputs(buffer, fichier);

   fclose(fichier);
}

static void persister_participants(const char* nom_conversation, const char *participant)
{
   char buffer[BUF_SIZE];
   strncpy(buffer, nom_conversation, BUF_SIZE-1);
   strncat(buffer, "_participants.txt", BUF_SIZE-1);
   FILE* fichier = fopen(buffer, "a");

   strncpy(buffer, participant, BUF_SIZE-1);
   strncat(buffer, "\n",BUF_SIZE-1);

   //Ecriture dans le fichier
   
   fputs(buffer, fichier);
   fclose(fichier);
}

static void persister_historique(const char* nom_conversation, Historique *historique)
{
   char buffer[BUF_SIZE];
   
   //Ouverture du fichier correspondant à la conversation

   strncpy(buffer, nom_conversation, BUF_SIZE-1);
   strncat(buffer, "_historique.txt", sizeof buffer - strlen(buffer) - 1);
   FILE* fichier = fopen(buffer, "a");

   //Mise en forme des données à stocker pour chaque message (historique)
   printf("juste ici\n" );
   printf("historique expéditeur : %s\n", historique->expediteur);

   strncpy(buffer, historique->expediteur, BUF_SIZE-1);
   strncat(buffer, ";", BUF_SIZE-1);
   strncat(buffer, historique->message, BUF_SIZE-1);
   strncat(buffer, "\n", BUF_SIZE-1);
   fputs(buffer, fichier);

   //Fermer le fichier
   fclose(fichier);
}

static void afficher_client(Client *client)
{
   printf("Nom :%s - Conversation actuelle :%s - index %d\r\n", client->name, client->conversation_actuelle->nom, client->actual_conversation);
}

//Test pour le serveur
static void afficher_historique(Conversation *conversation)
{
   printf("---Debut affichage historique\r\n");
   Historique* message = conversation->debut_historique;
   while(message != NULL)
   {
      printf("Message : '%s'\r\n", message->message);
      message = message->message_suivant;
   }
   printf("---Fin affichage historique\r\n");
}

//Test d'affichage pour les clients
static void afficher_clients(Client *clients, int nbClients)
{
   printf("---Debut affichage clients\r\n");
   printf("Nombre de clients enregistrés : %d\r\n", nbClients);

   int i;
   for(i=0;i<nbClients;i++)
   {
      printf("Nom : %s     | MdP : %s\r\n", clients[i].name,clients[i].password);
   }
   printf("---Fin affichage clients\r\n");
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
