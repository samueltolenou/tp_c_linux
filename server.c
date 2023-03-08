#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

// Get char ascii value from terminal : printf %d\\n \'r

struct stat st = {0};

struct User {
    int id;
    char *login;
};

struct AcceptedSocket {
    int socket_fd;
    struct sockaddr_in address;
    int error;
    bool accepted;
    struct User user;
};

#define MAX_LENGTH 1024
static const char ID_FILE[] = "id.txt";
static const char CHAT_FOLDER[] = "chats";
static const char USER_FILE[] = "users.txt";
static const char LOGIN_FILE[] = "login.txt";
static const char UNREAD_MSG[] = "unread.txt";

int port;
char *p = NULL;
char line[MAX_LENGTH];
char buffer[MAX_LENGTH];
char response[MAX_LENGTH];
int acceptedSocketsCount = 0;

struct AcceptedSocket *invalidSocket;
struct AcceptedSocket acceptedSockets[10];

/**
 * Accept an client/user incoming connection and return connection data
 *
 * @author nfassinou
 */
struct AcceptedSocket *accepter_un_connexion(int);

/**
 * Listener for accepting incoming connections
 * @param socketFd
 * @author Groupe 7
 */
void accepter_connections_entrantes(int);

/**
 * Process incoming data for a particular client socket
 * @param acceptedSocket
 * @author Groupe 7
 */
void dialoguer_avec_un_client(struct AcceptedSocket *);

/**
 * Process user registration
 * @param _socket &nbsp;Client socket file descriptor
 * @author Groupe 7
 */
void traitement_creation_compte(int);

/**
 * Listing des comptes utilisateurs créés
 * @author Groupe 7
 */
void traitement_liste_des_comptes(int);

/**
 * Processus d'envoi de messages a des utilisateurs
 * @author Groupe 7
 */
void traitement_envoi_des_messages(int);

/**
 * Chat instantané avec un utilisateur
 * @author Groupe 7
 */
void traitement_discussion_instantanee(int);

/**
 * Create a folder to store all chats for every user connected
 * @author Groupe 7
 */
void creation_dossier(char *);

/**
 * Util function for char replacement
 * @return
 * @author Groupe 7
 */
char *str_replace(char *, char, char);

/**
 * Suppress all leading white spaces in a char*
 * @return
 * @author Groupe 7
 */
char *trim_white_space(char *);

/**
 * Générer la date et heure au format francais
 * @return
 * @author Groupe 7
 */
char *get_date_time();

/**
 * Obtenir le repertoire utilisateur a partir de son ID
 * @return
 * @author Groupe 7
 */
char *get_dir(int);

/**
 * Listing des sockets actives
 * @author Groupe 7
 */
void lister_les_sockets();

/**
 * Util function to convert a char* to an Integer
 * @return
 * @author Groupe 7
 */
int str_to_int(char *);

/**
 * Création d'un compte utilisateur avec persistence des données dans un fichier
 */
void sauver_donnees_utilisateur(int, char *);

/**
 *
 * @link
 * @return
 * @author Groupe 7
 */
bool verifier_existence_utilisateur(char *);

/**
 * Util function to generate a new User ID based on file stored on server side
 * @return
 * @author Groupe 7
 */
int generation_id_utilisateur();

/**
 * Sauver les messages offline dans le fichier UNREAD.TXT de l'utilisateur destinataire
 * @author Groupe 7
 */
void sauver_messages_envoyes(char *, char *, char *);

/**
 * Lire le login de l'utilisateur a partir de son ID
 * @return
 * @author Groupe 7
 */
char *recuperer_login_utilisateur(char *);

bool verifier_socket_client(int);

void traitement_lecture_messages(int);

void traitement_authentification(int);


int main(int argc, char *argv[]) {


    if (argc < 2) {
        puts("Can not launch server : port number missing.");
        exit(1);
    }

    port = (int) strtol(argv[1], &p, 10);
    if (*p != '\0' || errno != 0) {
        printf("Invalid port.\n");
        exit(2);
    }

    int serverSocket;
    struct sockaddr_in serverAddress;
    invalidSocket = malloc(sizeof(struct AcceptedSocket));
    invalidSocket->socket_fd = 0;

    //Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        printf("Could not create socket");
        return 1;
    }
    puts("Server socket created successfully");

    //Prepare the server address structure
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    //Bind
    if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        //print the error content
        perror("bind failed. Error");
        return 1;
    }
    snprintf(buffer, sizeof(buffer), "Server bound on port %d", port);
    puts(buffer);

    listen(serverSocket, 3);
    puts("Waiting for incoming connections...");

    creation_dossier(CHAT_FOLDER);
    accepter_connections_entrantes(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);
    exit(EXIT_SUCCESS);
}

void creation_dossier(char *dir) {
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0777);
    }
}

void accepter_connections_entrantes(int socketFd) {
    while (true) {
        struct AcceptedSocket *clientSocket = accepter_un_connexion(socketFd);
        if (clientSocket == NULL) continue;
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;
        //lister_les_sockets();
        if (fork()) {
            close(socketFd);
            dialoguer_avec_un_client(clientSocket);
        }
    }
}

void dialoguer_avec_un_client(struct AcceptedSocket *acceptedSocket) {
    bool looop = true;
    while (looop) {
        ssize_t count = recv(acceptedSocket->socket_fd, response, 1024, 0);
        response[count] = 0;
        if (strcmp(response, "ins") == 0)
            traitement_creation_compte(acceptedSocket->socket_fd);

        if (strcmp(response, "auth") == 0)
            traitement_authentification(acceptedSocket->socket_fd);

        if (strcmp(response, "send") == 0)
            traitement_envoi_des_messages(acceptedSocket->socket_fd);

        if (strcmp(response, "lst") == 0)
            traitement_liste_des_comptes(acceptedSocket->socket_fd);

        if (strcmp(response, "chat") == 0)
            traitement_discussion_instantanee(acceptedSocket->socket_fd);

        if (strcmp(response, "read") == 0)
            traitement_lecture_messages(acceptedSocket->socket_fd);

        if (strcmp(response, "exit") == 0)
            looop = verifier_socket_client(acceptedSocket->socket_fd);

    }
    close(acceptedSocket->socket_fd);
}

struct AcceptedSocket *accepter_un_connexion(int socketDesc) {
    struct sockaddr_in client_addr;
    int length = sizeof(struct sockaddr_in);
    int client_socket = accept(socketDesc, &client_addr, &length);
    bool accepted = client_socket > 0;
    if (!accepted) return NULL;

    struct AcceptedSocket *acceptedSocket = malloc(sizeof(struct AcceptedSocket));
    acceptedSocket->address = client_addr;
    acceptedSocket->socket_fd = client_socket;
    acceptedSocket->accepted = accepted;

    if (!acceptedSocket->accepted)
        acceptedSocket->error = client_socket;

    // formattage de l'affichage des données de connexion
    sprintf(buffer, "Client #%d from %s:%d", client_socket, inet_ntoa(acceptedSocket->address.sin_addr),
            ntohs(acceptedSocket->address.sin_port));
    puts(buffer);
    return acceptedSocket;
}

// Gestion des options choisies par l'utilisateur (dialogue)
void traitement_authentification(int _socket) {
    FILE *fd;
    strcpy(buffer, "Entrez votre ID : ");
    send(_socket, buffer, strlen(buffer), 0);
    long cnt = recv(_socket, response, MAX_LENGTH, 0);
    response[cnt] = 0;
    int user_id = atoi(response);
    char *file = get_dir(user_id);
    asprintf(&file, "%s/%s", file, LOGIN_FILE);
    fd = fopen(file, "r");
    if (fd == NULL) {
        strcpy(buffer, "bad");
        send(_socket, buffer, strlen(buffer), 0);
        return;
    }
    // Mise a jour de la socket client
    char *login = recuperer_login_utilisateur(response);
    struct User *user = malloc(sizeof(struct User));
    user->id = user_id;
    user->login = login;

    for (int i = 0; i < acceptedSocketsCount; i++) {
        int sk = acceptedSockets[i].socket_fd;
        if (sk != _socket) continue;
        acceptedSockets[i].user = *user;
    }
    strcpy(buffer, "okay");
    fclose(fd);
    send(_socket, buffer, strlen(buffer), 0);
    lister_les_sockets();
}

void traitement_lecture_messages(int const _socket) {
    FILE *fd = NULL;
    struct User *messageUser = malloc(sizeof(struct User));
    
    for (int i = 0; i < acceptedSocketsCount; i++) {
        int sk = acceptedSockets[i].socket_fd;
        if (sk != _socket) continue;
         *messageUser =acceptedSockets[i].user ;
    }
    
    char path[1024];
    strcpy(line, "");
    snprintf(path, sizeof(path), "%s/%d/%s",CHAT_FOLDER,messageUser->id,UNREAD_MSG);
    puts(path);
   
       fd = fopen(path, "r");
    puts("ok");
       if (fd == NULL) sprintf(buffer, "Aucun Message en attente.");
       else {
           puts("good");
           sprintf(buffer, "\nMessage enregistre :\n");
           while (fgets(line, MAX_LENGTH, fd)) {
               sprintf(buffer, "%s   %s", buffer, line);
               puts(line);
           }
           puts(buffer);
       }
       fclose(fd);
    puts("end");
    send(_socket, buffer, strlen(buffer), 0);

}

void traitement_creation_compte(int _socket) {
    char *login = NULL;
    strcpy(buffer, "Entrez votre pseudo : ");
    send(_socket, buffer, strlen(buffer), 0);
    long cnt = recv(_socket, response, MAX_LENGTH, 0);
    response[cnt] = 0;
    login = trim_white_space(response);
    int user_id = generation_id_utilisateur();
    /*if (verifier_existence_utilisateur(login)) {
        sprintf(buffer, "User <%s> already exists.", login);
        send(_socket, buffer, strlen(buffer), 0);
        return;
    }*/
    sprintf(buffer, "Bienvenue dans le chat, %s (ID : %d)", login, user_id);
    send(_socket, buffer, strlen(buffer), 0);
    creation_dossier(get_dir(user_id));
    sauver_donnees_utilisateur(user_id, login);
}

void traitement_envoi_des_messages(int _socket) {
    traitement_liste_des_comptes(_socket);
    ssize_t cnt;
    int pos = 0;
    char *token;
    char tab[3][MAX_LENGTH] = {"", "", ""};
    while (true) {
        cnt = recv(_socket, response, MAX_LENGTH, 0);
        response[cnt] = 0;
        if (cnt <= 0) continue;
        if (strcmp(response, "end") == 0) break;
        pos = 0;
        puts(response);
        token = strtok(response, "|");
        strcpy(tab[pos], token);
        while (token != NULL) {
            pos++;
            token = strtok(NULL, "|");
            if (pos > 2) break;
            strcpy(tab[pos], token);
        }
        if (pos < 2) continue;
        sauver_messages_envoyes(tab[0], tab[1], tab[2]);
    }
    sprintf(buffer, "Appuyez pour continuer...");
    send(_socket, buffer, strlen(buffer), 0);
}

void traitement_liste_des_comptes(int _socket) {
    FILE *textfile;
    strcpy(line, "");
    textfile = fopen(USER_FILE, "r");
    if (textfile == NULL) sprintf(buffer, "Aucun utilisateur enregistré.");
    else {
        sprintf(buffer, "\nUtilisateurs inscrits :");
        while (fgets(line, MAX_LENGTH, textfile)) {
            sprintf(buffer, "%s   %s", buffer, line);
        }
    }
    fclose(textfile);
    send(_socket, buffer, strlen(buffer), 0);
}

void traitement_discussion_instantanee(int _socket) {
    FILE *fd = NULL;
    int last_id = 0;
    strcpy(buffer, "Entrez votre ID : ");
    send(_socket, buffer, strlen(buffer), 0);
    long cnt = recv(_socket, response, MAX_LENGTH, 0);
    response[cnt] = 0;
    int user_id = atoi(response);// str_to_int(response);
    sprintf(buffer, "Let's find this User ID #%d...", user_id);
    puts(buffer);
    fd = fopen(ID_FILE, "r");
    if (fd != NULL) {
        fscanf(fd, "%d", &last_id);
        fclose(fd);
    }
    bool okay = last_id != 0 && (user_id >= 1000 && user_id <= last_id);
    if (!okay) {
        sprintf(buffer, "no_user");
        send(_socket, buffer, strlen(buffer), 0);
        return;
    }
    sprintf(buffer, "User ID #%d found.", user_id);
    puts(buffer);
    sprintf(buffer, "Liste des connectés : \n");
    int connected = 0;
    for (int i = 0; i < acceptedSocketsCount; i++) {
        int curr_socket = acceptedSockets[i].socket_fd;
        if (curr_socket == _socket) continue;
        connected++;
        sprintf(
            buffer, "%s   %d) - %s (%d)\n", buffer, curr_socket,
            acceptedSockets[i].user.login,
            acceptedSockets[i].user.id
        );
    }
    if (connected == 0) {
        sprintf(buffer, "no_connected");
        send(_socket, buffer, strlen(buffer), 0);
        return;
    }
    sprintf(response, "%d users connected.", connected);
    puts(response);

    send(_socket, buffer, strlen(buffer), 0);
    cnt = recv(_socket, response, MAX_LENGTH, 0);
    response[cnt] = 0;
    int friend_socket = atoi(response);
    sprintf(buffer, "User socket : %d", friend_socket);
    puts(buffer);
    strcpy(buffer, "Vous pouvez démarrer. end pour finir");
    send(_socket, buffer, strlen(buffer), 0);
    while (true) {
        cnt = recv(_socket, response, MAX_LENGTH, 0);
        response[cnt] = 0;
        if (cnt <= 0) continue;
        if (strcmp(response, "end") == 0) {
            sprintf(buffer, "#%d a quitté la discussion.", _socket);
            puts(buffer);
            break;
        }
        sprintf(buffer, "#%d sent > %s", _socket, response);
        puts(buffer);
    }
}

bool verifier_socket_client(int _socket) {
    sprintf(buffer, "Client #%d has gone 👋!", _socket);
    for (int i = 0; i < acceptedSocketsCount; i++) {
        int curr_socket = acceptedSockets[i].socket_fd;
        if (curr_socket == _socket) {
            acceptedSockets[i] = *invalidSocket;
        }
    }
    lister_les_sockets();
    puts(buffer);
    return false;
}

// fonctions utilitaires
void sauver_messages_envoyes(char *sender, char *receiver, char *message) {
    char *filename;
    FILE *fd = NULL;
    char *login = recuperer_login_utilisateur(sender);
    char *folder = get_dir(atoi(receiver));
    asprintf(&filename, "%s/%s", folder, UNREAD_MSG);
    fd = fopen(filename, "a+");
    char *dt = get_date_time();
    fprintf(fd, "%s > %s (%s)\n", login, message, dt);
    fclose(fd);
}

char *get_dir(int user_id) {
    char *dir;
    asprintf(&dir, "%s/%d", CHAT_FOLDER, user_id);
    return dir;
}

char *trim_white_space(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char) *str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

char *str_replace(char *str, char find, char replace) {
    str = trim_white_space(str);
    char *current_pos = strchr(str, find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos, find);
    }
    return str;
}

char *get_date_time() {
    char *stm;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    asprintf(&stm, "%02d/%02d/%d %02d:%02d:%02d",
             tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    return stm;
}

void lister_les_sockets() {
    int n = 0;
    struct User *user = malloc(sizeof(struct User));
    strcpy(buffer, "********************\n");
    for (int i = 0; i < acceptedSocketsCount; i++) {
        int sk = acceptedSockets[i].socket_fd;
        if (sk > 0) {
            n++;
            sprintf(buffer, "%sSocket #%d active", buffer, sk);
            *user = acceptedSockets[i].user;
            if (acceptedSockets[i].user.login == NULL)
                sprintf(buffer, "%s\n", buffer);
            else
                sprintf(buffer, "%s (%d - %s)\n", buffer, user->id, user->login);
        }
    }
    if (n > 0) sprintf(buffer, "%s********************\n", buffer);
    else sprintf(buffer, "No user connected.");
    puts(buffer);
    free(user);
}

int str_to_int(char *str) {
    p = NULL;
    long numValue = strtol(str, &p, 10);
    if (*p != '\0' || errno != 0)
        return 0;
    int digit = (int) numValue;
    return digit;
}

// fonctions de gestion User
int generation_id_utilisateur() {
    int id = 1000; // depart
    FILE *fd = NULL;
    fd = fopen(ID_FILE, "r");
    if (fd != NULL) {
        fscanf(fd, "%d", &id);
        fclose(fd);
        id++;
    }
    fd = fopen(ID_FILE, "w");
    fprintf(fd, "%d", id);
    fclose(fd);
    return id;
}

char *recuperer_login_utilisateur(char *id) {
    FILE *fd = NULL;
    strcpy(line, "");
    char *file = get_dir(atoi(id));
    asprintf(&file, "%s/%s", file, LOGIN_FILE);
    fd = fopen(file, "r");
    fgets(line, MAX_LENGTH, fd);
    fclose(fd);
    return line;
}

void sauver_donnees_utilisateur(int id, char *user) {
    char *f;
    FILE *fd = NULL;
    fd = fopen(USER_FILE, "a+");
    fprintf(fd, "\n%d\t%s", id, user);
    fclose(fd);

    // Save user login
    char *dir = get_dir(id);
    asprintf(&f, "%s/%s", dir, LOGIN_FILE);
    fd = fopen(f, "a+");
    fprintf(fd, "%s", user);
    fclose(fd);
}

bool verifier_existence_utilisateur(char *user) {
    FILE *fp;
    char ch;
    char word[50];
    int pointer = 0;
    int count = 0;
    fp = fopen(USER_FILE, "r");
    if (fp == NULL) {
        fclose(fp);
        return false;
    }
    do {
        ch = fscanf(fp, "%s", word);
        if (strcmp(word, user) == 0) count++;
        pointer++;
    } while (ch != EOF);
    fclose(fp);
    return count != 0;
}
