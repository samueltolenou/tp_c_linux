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
#define INSC 105
#define SEND 115
#define READ 114
#define QUIT 113
#define CHAT 99
#define LST 108
#define AUTH 97
#define LOGOUT 100

int port;
int quit = 0;
char *ip = "";
char *p = NULL;
int clientSocket;
char *input = NULL;
size_t inputSize = 0;
char message[MAX_LENGTH];
static const int OPTION_VALUES[] = {
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
int display_menu_option();

/**
 * Exit de l'application
 */
void exit_chat();

/**
 * Module de creation des comptes utilisateurs
 * Le key code a envoyer au serveur est 'ins'
 */
void register_user();

/**
 * Module d'envoi des messages on offline
 * Dans ce contexte, le compte utilisateur doit exister
 * Les messages seront stockés dans le fichier unread.txt de l'utilisateur
 * Le key code a envoyer au serveur est 'send'
 */
void send_message();

/**
 * Module de chat instantané avec un utilisateur que le client
 * aurait choisi avec vérification sur le serveur si l'interlocuteur
 * est en ligne
 * Le key code a envoyer au serveur est 'chat'
 */
void chat_with_friend();

/**
 * Code du bloc d'affichage de la liste des utilisateurs ayant créé
 * un compte sur le reseau
 * Le key code a envoyer au serveur est 'lst'
 */
void show_user_list();

/**
 * Module d'authentification Utilisateur
 */
void user_authentication();

void waiting();

ssize_t read_stdin(const char *);

void okay(const char *);

void error(const char *);

/**
 * Module exécuté le thread principal du client
 */
void main_thread();

void read_offline_message();

bool is_option_valide(int);

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
        main_thread();
        if (quit == 1) break;
    }
    exit_chat();
}

void main_thread() {
    int option = display_menu_option();
    switch (option) {
        case INSC:
            register_user();
            break;
        case AUTH:
            user_authentication();
            break;
        case CHAT:
            chat_with_friend();
            break;
        case SEND:
            send_message();
            break;
        case LST:
            show_user_list();
            break;
        case READ:
            read_offline_message();
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

int display_menu_option() {
    int ch;
    while (true) {
        // present the menu, accept the user's choice
        printf("\n\nMenu des options :");
        if(isConnected) {
            printf("\n    S/s) Envoyer un message a un ami.");
            printf("\n    C/c) Discuter avec un ami...");
            printf("\n    R/r) Lire mes messages.");
            printf("\n    L/l) Liste des comptes.");
            printf("\n    D/d) Déconnexion (Logout).");
        } else {
            printf("\n    I/i) Créer un nouveau compte (Inscription).");
            printf("\n    A/a) Authentification (Login).");
        }
        printf("\n    Q/q) Quitter le menu.");
        read_stdin("\n\nEntrer votre choix : ");
        ch = tolower(*input);
        //printf("%d", ch);
        if(!is_option_valide(ch)) continue;
        break;
    }
    return ch;
}

void register_user() {
    sprintf(message, "ins");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    read_stdin(message);
    sprintf(message, "%s", input);
    send(clientSocket, input, strlen(input), 0);
    count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    puts("Inscription terminée.");
}

void user_authentication() {
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
    //read_stdin(message);

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
    waiting();
}

void show_user_list() {
    sprintf(message, "lst");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    printf("Appuyez pour continuer...");
    getchar();
}

void read_offline_message() {
    sprintf(message, "read");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    sprintf(message, "%s", user_id);
    send(clientSocket, message, strlen(message), 0);

    count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    waiting();
}

void chat_with_friend() {
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
    read_stdin("Entrez l'ID de votre ami : ");
    sprintf(message, "%s", input);
    send(clientSocket, message, strlen(message), 0);
    recv(clientSocket, message, MAX_LENGTH, 0);
    puts(message);
    bool chating = true;
    while (chating) {
        read_stdin("$@ > ");
        chating = strcmp(input, "end") != 0;
        send(clientSocket, input, strlen(input), 0);
    }
}

void send_message() {
    char sender[10] = "";
    char receiver[10] = "";
    sprintf(message, "send");
    send(clientSocket, message, strlen(message), 0);
    long count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    puts(message);
    read_stdin("Votre ID : ");
    sprintf(sender, "%s", input);
    read_stdin("Son ID : ");
    sprintf(receiver, "%s", input);
    puts("Taper 'end' pour arreter");
    while (true) {
        read_stdin("$@ > ");
        if (strcmp(input, "end") == 0) {
            send(clientSocket, "end", 3, 0);
            break;
        }
        sprintf(message, "%s|%s|%s", sender, receiver, input);
        send(clientSocket, message, strlen(message), 0);
    }
    count = recv(clientSocket, message, MAX_LENGTH, 0);
    message[count] = 0;
    printf("%s", message); getchar();
}

void exit_chat() {
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

ssize_t read_stdin(const char *prompt) {
    printf("%s", prompt);
    ssize_t count = getline(&input, &inputSize, stdin);
    input[count - 1] = 0;
    return count;
}

bool is_option_valide(int value) {
    int arrLen = sizeof OPTION_VALUES / sizeof OPTION_VALUES[0];
    bool isPresent = false;

    for (int i = 0; i < arrLen; i++) {
        if (OPTION_VALUES[i] == value) {
            isPresent = true;
            break;
        }
    }
    return isPresent;
}

void waiting() {
    printf("Appuyez pour continuer...");
    getchar();
}
//system("clear");
