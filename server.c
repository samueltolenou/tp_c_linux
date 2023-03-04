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
 * @return
 * @author nfassinou
 */
struct AcceptedSocket *accept_connection(int);

/**
 * Listener for accepting incoming connections
 * @param socketFd
 * @author Groupe 7
 */
void accepting_incoming_connections(int);

/**
 * Process incoming data for a particular client socket
 * @param acceptedSocket
 * @author Groupe 7
 */
void process_incoming_data(struct AcceptedSocket *);

/**
 * Process user registration
 * @param _socket &nbsp;Client socket file descriptor
 * @author Groupe 7
 */
void process_user_registration(int);

/**
 * Listing des comptes utilisateurs créés
 * @author Groupe 7
 */
void process_user_listing(int);

/**
 * Processus d'envoi de messages a des utilisateurs
 * @author Groupe 7
 */
void process_send_message(int);

/**
 * Chat instantané avec un utilisateur
 * @author Groupe 7
 */
void process_chat_with_user(int);

/**
 * Create a folder to store all chats for every user connected
 * @author Groupe 7
 */
void create_chat_folder(char *);

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
void list_socket();

/**
 * Util function to convert a char* to an Integer
 * @return
 * @author Groupe 7
 */
int str_to_int(char *);

/**
 * Création d'un compte utilisateur avec persistence des données dans un fichier
 */
void save_user_account(int, char *);

/**
 *
 * @link https://www.tutorialspoint.com/learn_c_by_examples/program_to_find_text_occurence_in_file_in_c.htm
 * @return
 * @author Groupe 7
 */
bool user_exist(char *);

/**
 * Util function to generate a new User ID based on file stored on server side
 * @return
 * @author Groupe 7
 */
int generate_user_id();

/**
 * Sauver les messages offline dans le fichier UNREAD.TXT de l'utilisateur destinataire
 * @author Groupe 7
 */
void persist_message(char *, char *, char *);

/**
 * Lire le login de l'utilisateur a partir de son ID
 * @return
 * @author Groupe 7
 */
char *get_user_login(char *);

bool exit_client(int);

void process_read_user_message(int);

void process_user_authentication(int);

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
    create_chat_folder(CHAT_FOLDER);
    accepting_incoming_connections(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);
    exit(EXIT_SUCCESS);
}

void create_chat_folder(char *dir) {
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0777);
    }
}

void accepting_incoming_connections(int socketFd) {
    while (true) {
        struct AcceptedSocket *clientSocket = accept_connection(socketFd);
        if (clientSocket == NULL) continue;
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;
        //list_socket();
        if (fork()) {
            close(socketFd);
            process_incoming_data(clientSocket);
        }
    }
}

void process_incoming_data(struct AcceptedSocket *acceptedSocket) {
    bool looop = true;
    while (looop) {
        ssize_t count = recv(acceptedSocket->socket_fd, response, 1024, 0);
        response[count] = 0;
        if (strcmp(response, "ins") == 0)
            process_user_registration(acceptedSocket->socket_fd);

        if (strcmp(response, "auth") == 0)
            process_user_authentication(acceptedSocket->socket_fd);

        if (strcmp(response, "send") == 0)
            process_send_message(acceptedSocket->socket_fd);

        if (strcmp(response, "lst") == 0)
            process_user_listing(acceptedSocket->socket_fd);

        if (strcmp(response, "chat") == 0)
            process_chat_with_user(acceptedSocket->socket_fd);

        if (strcmp(response, "read") == 0)
            process_read_user_message(acceptedSocket->socket_fd);

        if (strcmp(response, "exit") == 0)
            looop = exit_client(acceptedSocket->socket_fd);

    }
    close(acceptedSocket->socket_fd);
}

struct AcceptedSocket *accept_connection(int socketDesc) {
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

    sprintf(buffer, "Client #%d from %s:%d", client_socket, inet_ntoa(acceptedSocket->address.sin_addr),
            ntohs(acceptedSocket->address.sin_port));
    puts(buffer);
    return acceptedSocket;
}

// Gestion des options choisies par l'utilisateur (dialogue)
void process_user_authentication(int _socket) {
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
    char *login = get_user_login(response);
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
    list_socket();
}

void process_read_user_message(int _socket) {
    FILE *fd = NULL;
    strcpy(buffer, "Entrez votre ID : ");
    send(_socket, buffer, strlen(buffer), 0);
    long cnt = recv(_socket, response, MAX_LENGTH, 0);
    response[cnt] = 0;
    int user_id = atoi(response);
    char *file = get_dir(user_id);
    asprintf(&file, "%s/%s", file, UNREAD_MSG);
    strcpy(buffer, "Formattage des messages non lus...");
    send(_socket, buffer, strlen(buffer), 0);
    puts(file);
}

void process_user_registration(int _socket) {
    char *login = NULL;
    strcpy(buffer, "Entrez votre pseudo : ");
    send(_socket, buffer, strlen(buffer), 0);
    long cnt = recv(_socket, response, MAX_LENGTH, 0);
    response[cnt] = 0;
    login = trim_white_space(response);
    int user_id = generate_user_id();
    /*if (user_exist(login)) {
        sprintf(buffer, "User <%s> already exists.", login);
        send(_socket, buffer, strlen(buffer), 0);
        return;
    }*/
    sprintf(buffer, "Bienvenue dans le chat, %s (ID : %d)", login, user_id);
    send(_socket, buffer, strlen(buffer), 0);
    create_chat_folder(get_dir(user_id));
    save_user_account(user_id, login);
}

void process_send_message(int _socket) {
    process_user_listing(_socket);
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
        persist_message(tab[0], tab[1], tab[2]);
    }
    sprintf(buffer, "Appuyez pour continuer...");
    send(_socket, buffer, strlen(buffer), 0);
}

void process_user_listing(int _socket) {
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

void process_chat_with_user(int _socket) {
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

bool exit_client(int _socket) {
    sprintf(buffer, "Client #%d has gone 👋!", _socket);
    for (int i = 0; i < acceptedSocketsCount; i++) {
        int curr_socket = acceptedSockets[i].socket_fd;
        if (curr_socket == _socket) {
            acceptedSockets[i] = *invalidSocket;
        }
    }
    list_socket();
    puts(buffer);
    return false;
}

// fonctions utilitaires
void persist_message(char *sender, char *receiver, char *message) {
    char *filename;
    FILE *fd = NULL;
    char *login = get_user_login(sender);
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

void list_socket() {
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
int generate_user_id() {
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

char *get_user_login(char *id) {
    FILE *fd = NULL;
    strcpy(line, "");
    char *file = get_dir(atoi(id));
    asprintf(&file, "%s/%s", file, LOGIN_FILE);
    fd = fopen(file, "r");
    fgets(line, MAX_LENGTH, fd);
    fclose(fd);
    return line;
}

void save_user_account(int id, char *user) {
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

bool user_exist(char *user) {
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