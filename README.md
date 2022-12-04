# TP de programmation réseau

Fichiers sources
-------------

* Makefile
* Clients
	* client2.c
	* client2.h
* Serveur
	* client2.h
	* conversation.h
	* historique.h
	* server2.c
	* server2.h

Fonctionnalités implémantées
-------------
Il s'agit d'un tchat de clients.
Une fois le client connecté (via un mot de passe), celui-ci peut décider de créer ou rejoindre un groupe de conversation.
Au sein d'un groupe de conversation, il est possible de recevoir et d'envoyer des messages à toutes les personnes actuellement connectées à la conversation.
Lors de la connection au groupe de conversation, l'historique de celui-ci est affiché.

Les données des utilisateurs (nom et mot de passe), les conversations et les messages qui y sont envoyés sont stockés dans des fichiers .txt tout au long de l'utilisation de l'application. Lors de la relance des serveurs, ces mêmes fichiers permettent de recréer l'ensemble des clients, conversations et historiques de conversation.

Guide d'utilisation
-------------
Pour afficher le menu d'aide : /help
Pour créer un groupe : /group nom_groupe
Pour inviter une personne dans le groupe (en y étant connecté) : /invite nom_personne
Pour rejoindre un groupe (seulement pour les clients ajoutés dans le groupe) : /join nom_groupe
Pour afficher les informations du groupe : /infos
Pour quitter le tchat : /quit

Compilation
-------------
Il est possible de compiler l'ensemble du programme via la commande "make" (crée les fichiers binaires client et server)
Pour supprimer les fichiers créés lors de la compilation ou l'utilisation du programme, on peut utiliser la commande "make clean" (supprimer client, server et tout fichier .txt)

En étant sur le même path que les fichiers créés par le makefile
Pour lancer le serveur : ./server
Pour lancer un client : ./client "IP utilisée" "Nom du client" (exemple ./client 127.0.0.1 Tom)
