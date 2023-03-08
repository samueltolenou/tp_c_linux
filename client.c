#include <stdio.h>    //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h>    //inet_addr
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LENGTH 1024
#define INSC 105        // i
#define SEND 115        // s
#define READ 114        // r
#define QUIT 113        // q
#define CHAT 99         // c
#define LST 108         // l
#define AUTH 97         // a
#define LOGOUT 100      // d

int port;
int quit = 0;
char *ip = "127.0.0.1";
char *p = NULL;
int clientSocket;
char *input = NULL;
size_t inputSize = 0;
char message[MAX_LENGTH];
static const int OPTIONS_VALIDES[] = {
        INSC, AUTH, LST, CHAT,
        SEND, READ, QUIT, LOGOUT
};
bool isConnected;
char *user_id = NULL;

/**
 * Affichage des menus d'options utilisateurs pour l'interaction dynamique
 * avec le client. Les requetes eront formulées et envoyées au serveur
 * en fonction effectuées.
 * @return
 */
int afficher_options_menu();

/**
 * Exit de l'application
 */
void quitter_application();

/**
 * Module de creation des comptes utilisateurs
 * Le key code a envoyer au serveur est 'ins'
 */
void creation_de_compte_utilisateur();

/**
 * Module d'envoi des messages on offline
 * Dans ce contexte, le compte utilisateur doit exister
 * Les messages seront stockés dans le fichier unread.txt de l'utilisateur
 * Le key code a envoyer au serveur est 'send'
 */
void envoyer_messages_hors_ligne();

/**
 * Module de chat instantané avec un utilisateur que le client
 * aurait choisi avec vérification sur le serveur si l'interlocuteur
 * est en ligne
 * Le key code a envoyer au serveur est 'chat'
 */
void discuter_avec_un_ami();

/**
 * Code du bloc d'affichage de la liste des utilisateurs ayant créé
 * un compte sur le reseau
 * Le key code a envoyer au serveur est 'lst'
 */
void afficher_liste_utilisateur();

/**
 * Module d'authentification Utilisateur
 */
void authentification_utilisateur();

/**
 * Mettre en attente l'application jusqu'a l'appui d'une touche
 */
void appuyer_touche_pour_continuer();

/**
 * Lire des données entrées au client et les stocker dans la variable
 * globale *input pour utisation utltérieure
 * Prends en parametre une chaine de caractères a afficher en prompt pour la saisie
 * @return
 */
ssize_t lire_une_ligne(const char *);

/**
 * Afficher un message de succès
 * Prends en parametre une chaine de caractères a afficher
 */
void okay(const char *);

/**
 * Afficher un message d'erreur
 * Prends en parametre une chaine de caractères à afficher
 */
void error(const char *);

/**
 * Module exécuté le thread principal du client
 */
void traitement_prinicpal();

/**
 * Module de lecture des messages envoyés d'autres amis lorsque le
 * demandeur (utilisateur courant) n'est pas connecté
 */
void lire_messages_hors_ligne();

/**
 * Module de vérification de la validité de l'option de menu
 * choisie par l'utilisateur
 * Prends en parametre un entier représantant le choix utilisateur
 * @return
 */
bool verifier_choix_valide(int);

/**
 * Entrée prinicpale de l'application Client
 * @param argc Nombre d'arguments en ligne de ommande
 * @param argv Tableau des arguments saisis en ligne de commande
 * @return
 */
int main(int argc, char *argv[]) {
    char *str_port;
    switch (argc) {
        case 2:
            str_port = argv[1];
            break;
        case 3:
            ip = argv[1];
            str_port = argv[2];
            break;
        default:
            error("Bad command : ./client [ip] port");
            puts("ip : server IP address. If not set, client will run on localhost (127.0.0.1)");
            puts("port : port number open on the server ");
            exit(1);
    }

    port = (int) strtol(str_port, &p, 10);
    if (*p != '\0' || errno != 0) {
        error("Invalid port.");
        exit(2);
    }

    struct sockaddr_in serverAddress;

    //Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        okay("Could not create socket");
        return 1;
    }
    okay("Client socket created successfully");

    //Prepare the server address structure
    serverAddress.sin_addr.s_addr = inet_addr(ip);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    //Connect to remote serverAddress
    int conn = connect(clientSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (conn < 0) {
        error("Client could not connect to server");
        return 1;
    }

    okay("Client connected successfully");
    isConnected = false;
    while (true) {
        traitement_prinicpal();
        if (quit == 1) break;
    }
    quitter_application();
}

void traitement_prinicpal() {
    int option = afficher_options_menu();
    switch (option) {
        case INSC:
            creation_de_compte_utilisateur();
            break;
        case AUTH:
            authentification_utilisateur();
            break;
        case CHAT:
            discuter_avec_un_ami();
            break;
        case SEND:
            envoyer_messages_hors_ligne();
            break;
        case LST:
            afficher_liste_utilisateur();
            break;
        case READ:
            lire_messages_hors_ligne();
            break;
        case LOGOUT:
            isConnected = false;
            break;
        case QUIT:
            quit = 1;
            break;
        default:
            break;
    }
}

int afficher_options_menu() {
    int ch;
    while (true) {
        // present the menu, accept the user's choice
        printf("\n\nMenu des options :");
        if(isConnected) { // authentifiée
            printf("\n    S/s) Envoyer un message a un ami.");
            printf("\n    C/c) Discuter avec un ami...");
            printf("\n    R/r) Lire mes messages.");
            printf("\n    L/l) Liste des comptes.");
            printf("\n    D/d) Déconnexion (Logout).");
        } else { // non authentifée
            printf("\n    I/i) Créer un nouveau compte (Inscription).");
            printf("\n    A/a) Authentification (Login).");
        }
        printf("\n    Q/q) Quitter le menu.");
        lire_une_ligne("\n\nEntrer votre choix : ");
        ch = tolower(*input);
        //printf("%d", ch);
        if(!verifier_choix_valide(ch)) continue;
        break;
    }
    return ch;
}

void creation_de_compte_utilisateur() {
    sprintf(message, "ins");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    lire_une_ligne(message);
    sprintf(message, "%s", input);
    send(clientSocket, input, strlen(input), 0);
    count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    puts("Inscription terminée.");
}

void authentification_utilisateur() {
    sprintf(message, "auth");
    // envoi de la requete d'authentification
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    // prompt pour saisir le login
    printf("%s", message);
    size_t ss = 0;
    count = getline(&user_id, &ss, stdin);
    user_id[count - 1] = 0;
    //lire_une_ligne(message);

    // Send USer ID for checking
    send(clientSocket, user_id, strlen(user_id), 0);
    count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    if(count == 0) return;
    if(strcmp(message, "bad") == 0) {
        sprintf(message, "User ID <%s> non valide. Bien vouloir reesayer ou créer un compte", input);
    } else {
        sprintf(message, "Connexion effectuée avec succès.");
        isConnected = true;
        //puts(user_id);
    }
    puts(message);
    appuyer_touche_pour_continuer();
}

void afficher_liste_utilisateur() {
    sprintf(message, "lst");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    printf("Appuyez pour continuer...");
    getchar();
}

void lire_messages_hors_ligne() {
    sprintf(message, "read");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    appuyer_touche_pour_continuer();
}

void discuter_avec_un_ami() {
    sprintf(message, "chat");
    send(clientSocket, message, strlen(message), 0);
    // prompt received
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    sprintf(message, "%s", user_id);
    send(clientSocket, message, strlen(message), 0);
    // Connected list received
    count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    if (strcmp(message, "no_user") == 0) {
        sprintf(message, "User <%s> not found.", input);
        puts(message);
        return;
    }
    if (strcmp(message, "no_connected") == 0) {
        puts("Pas d'utilisateur connectés.");
        return;
    }
    // prompt for entering friend id
    puts(message);
    lire_une_ligne("Entrez l'ID de votre ami : ");
    sprintf(message, "%s", input);
    send(clientSocket, message, strlen(message), 0);
    recv(clientSocket, message, MAX_LENGTH, 0);
    puts(message);
    bool chating = true;
    while (chating) {
        lire_une_ligne("$@ > ");
        chating = strcmp(input, "end") != 0;
        send(clientSocket, input, strlen(input), 0);
    }
}

void envoyer_messages_hors_ligne() {
    char receiver[10] = "";
    sprintf(message, "send");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    lire_une_ligne("ID ami: ");
    sprintf(receiver, "%s", input);
    puts("Taper 'end' pour arreter");
    while (true) {
        lire_une_ligne("$@ > ");
        if (strcmp(input, "end") == 0) {
            send(clientSocket, "end", 3, 0);
            break;
        }
        sprintf(message, "%s|%s|%s", user_id, receiver, input);
        send(clientSocket, message, strlen(message), 0);
    }
    count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    printf("%s", message); getchar();
}

void quitter_application() {
    if (quit == 0) return;
    puts("Merci d'etre passé, bye 👋 et a la prochaine.");
    strcpy(message, "exit");
    send(clientSocket, message, sizeof(message), 0);
    close(clientSocket);
    exit(EXIT_SUCCESS);
}

void okay(const char *msg) {
    sprintf(message, "[OK] %s", msg);
    puts(message);
}

void error(const char *msg) {
    sprintf(message, "[ERR] %s", msg);
    puts(message);
}

ssize_t lire_une_ligne(const char *prompt) {
    printf("%s", prompt);
    ssize_t count = getline(&input, &inputSize, stdin);
    // recupérer tout sauf le dernier caracter qui ENTREE
    input[count - 1] = 0;
    return count;
}

bool verifier_choix_valide(int value) {
    int arrLen = sizeof OPTIONS_VALIDES / sizeof OPTIONS_VALIDES[0];
    bool isPresent = false;

    for (int i = 0; i < arrLen; i++) {
        if (OPTIONS_VALIDES[i] == value) {
            isPresent = true;
            break;
        }
    }
    return isPresent;
}

void appuyer_touche_pour_continuer() {
    printf("Appuyez pour continuer...");
    getchar();
}
//system("clear");
