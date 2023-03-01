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

struct stat st = {0};

struct AcceptedSocket {
    int socket_fd;
    struct sockaddr_in address;
    int error;
    bool accepted;
};

#define MAX_LINE_LENGTH 1000
static const char CHAT_FOLDER[] = "chats";
static const char LOGIN_FILE[] = "login.txt";
static const char ID_FILE[] = "id.txt";
struct AcceptedSocket *invalidSocket;

static const char USER_FILE[] = "users.txt";
char *p = NULL;
int port = 8888;
char buffer[1024];
char response[1024];
int acceptedSocketsCount = 0;

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
 * @author nfassinou
 */
void accepting_incoming_connections(int);

/**
 * Process incoming data for a particular client socket
 * @param acceptedSocket
 * @author nfassinou
 */
void process_incoming_data(struct AcceptedSocket *);

/**
 * Process user registration
 * @param _socket &nbsp;Client socket file descriptor
 * @author nfassinou
 */
void process_user_registration(int);

/**
 * Util function to convert a char* to an Integer
 * @return
 * @author nfassinou
 */
int str_to_int(char *);

/**
 * Util function to generate a new User ID based on file stored on server side
 * @return
 * @author nfassinou
 */
int generate_user_id();

/**
 * Create a folder to store all chats for every user connected
 * @author nfassinou
 */
void create_chat_folder(char *);

/**
 * Util function for char replacement
 * @return
 */
char *str_replace(char *, char, char);

/**
 * Suppress all leading white spaces in a char*
 * @return
 */
char *trim_white_space(char *);

void save_user_account(int, char *);

/**
 *
 * @link https://www.tutorialspoint.com/learn_c_by_examples/program_to_find_text_occurence_in_file_in_c.htm
 * @return
 */
bool user_exist(char *);

void process_chat_with_user(int);

char *get_dir(int);

void list_socket();

void process_user_listing(int);

int main() {//int argc , char *argv[])
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
        //print the error message
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
        list_socket();
        if (fork()) {
            close(socketFd);
            process_incoming_data(clientSocket);
        }
    }
}

void list_socket() {
    int n = 0;
    strcpy(buffer, "********************\n");
    for (int i=0; i<acceptedSocketsCount; i++) {
        int sk = acceptedSockets[i].socket_fd;
        if(sk <= 0) continue;
        n++;
        sprintf(buffer, "%sSocket #%d active\n", buffer, sk);
    }
    if(n>0) sprintf(buffer, "%s********************\n", buffer);
    else sprintf(buffer, "No user connected.");
    puts(buffer);
}

void process_incoming_data(struct AcceptedSocket *acceptedSocket) {
    bool looop = true;
    while (looop) {
        ssize_t count = recv(acceptedSocket->socket_fd, response, 1024, 0);
        response[count] = 0;
        if (strcmp(response, "ins") == 0) {
            process_user_registration(acceptedSocket->socket_fd);
        }
        if (strcmp(response, "lst") == 0) {
            process_user_listing(acceptedSocket->socket_fd);
        }
        if (strcmp(response, "chat") == 0) {
            process_chat_with_user(acceptedSocket->socket_fd);
        }
        if (strcmp(response, "exit") == 0) {
            looop = false;
            sprintf(buffer, "Client #%d has gone 👋!", acceptedSocket->socket_fd);
            for(int i = 0 ; i<acceptedSocketsCount ; i++){
                int curr_socket = acceptedSockets[i].socket_fd;
                if(curr_socket == acceptedSocket->socket_fd) {
                    acceptedSockets[i] = *invalidSocket;
                }
                list_socket();
            }
            puts(buffer);
        }
    }
    close(acceptedSocket->socket_fd);
}

void process_user_listing(int _socket) {
    FILE    *textfile;
    char    line[MAX_LINE_LENGTH];
    textfile = fopen(USER_FILE, "r");
    if(textfile == NULL) sprintf(buffer, "Aucun utilisateur enregistré.");
    else {
        sprintf(buffer, "\nUtilisateurs inscrits :");
        while(fgets(line, MAX_LINE_LENGTH, textfile)){
            sprintf(buffer, "%s   %s", buffer, line);
        }
    }
    fclose(textfile);
    send(_socket, buffer, strlen(buffer), 0);
}

void process_chat_with_user(int _socket) {
    FILE *fptr = NULL;
    int last_id = 0;
    strcpy(buffer, "Entrez votre ID : ");
    send(_socket, buffer, strlen(buffer), 0);
    long cnt = recv(_socket, response, 1024, 0);
    response[cnt] = 0;
    int user_id = atoi(response);// str_to_int(response);
    sprintf(buffer, "Let's find this User ID #%d...", user_id);
    puts(buffer);
    fptr = fopen(ID_FILE, "r");
    if (fptr != NULL) {
        fscanf(fptr, "%d", &last_id);
        fclose(fptr);
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
    for(int i = 0 ; i<acceptedSocketsCount ; i++){
        int curr_socket = acceptedSockets[i].socket_fd;
        if(curr_socket == _socket) continue;
        connected++;
        sprintf(buffer, "%s   %d) - \n", buffer, curr_socket);
    }
    if(connected==0) {
        sprintf(buffer, "no_connected");
        send(_socket, buffer, strlen(buffer), 0);
        return;
    }
    sprintf(response, "%d users connected.", connected);
    puts(response);

    send(_socket, buffer, strlen(buffer), 0);
    cnt = recv(_socket, response, 1024, 0);
    response[cnt] = 0;
    int friend_socket = atoi(response);
    sprintf(buffer, "User socket : %d", friend_socket);
    puts(buffer);
    strcpy(buffer, "Vous pouvez démarrer. end pour finir");
    send(_socket, buffer, strlen(buffer), 0);
    while (true) {
        cnt = recv(_socket, response, 1024, 0);
        response[cnt] = 0;
        if(cnt <= 0) continue;
        if(strcmp(response, "end") == 0) {
            sprintf(buffer, "#%d a quitté la discussion.", _socket);
            puts(buffer);
            break;
        }
        sprintf(buffer, "#%d sent > %s", _socket, response);
        puts(buffer);
    }
}

void process_user_registration(int _socket) {
    char *login = NULL;
    strcpy(buffer, "Entrez votre pseudo : ");
    send(_socket, buffer, strlen(buffer), 0);
    long cnt = recv(_socket, response, 1024, 0);
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

void save_user_account(int id, char *user) {
    char *f;
    FILE *fptr = NULL;
    fptr = fopen(USER_FILE, "a+");
    fprintf(fptr, "\n%d\t%s", id, user);
    fclose(fptr);

    // Save user login
    char* dir = get_dir(id);
    asprintf(&f, "%s/%s", dir, LOGIN_FILE);
    fptr = fopen(f, "a+");
    fprintf(fptr, "%s", user);
    fclose(fptr);
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

int str_to_int(char *str) {
    p = NULL;
    long numValue = strtol(str, &p, 10);
    if (*p != '\0' || errno != 0)
        return 0;
    int digit = (int) numValue;
    return digit;
}

int generate_user_id() {
    int id = 1000; // depart
    FILE *fptr = NULL;
    fptr = fopen(ID_FILE, "r");
    if (fptr != NULL) {
        fscanf(fptr, "%d", &id);
        fclose(fptr);
        id++;
    }
    fptr = fopen(ID_FILE, "w");
    fprintf(fptr, "%d", id);
    fclose(fptr);
    return id;
}