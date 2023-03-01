#include <stdio.h>	//printf
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define INSC 105
#define SEND 115
#define READ 114
#define QUIT 113
#define CHAT 99
#define LST 108

char message[1024];
int port = 8888;
char* ip = "127.0.0.1";
char *input = NULL;
size_t inputSize= 0;
int clientSocket;
int quit = 0;

int display_menu_option();

ssize_t read_stdin(const char *);

void okay(const char *);

void error(const char *);

void exit_chat();

void register_user();

void send_message();

void chat_with_friend();

void waiting();

void show_user_list();

void main_thread();

int main() //int argc , char *argv[])
{
    struct sockaddr_in serverAddress;

    //Create socket
    clientSocket = socket(AF_INET , SOCK_STREAM , 0);
    if (clientSocket < 0)
    {
        okay("Could not create socket");
        return 1;
    }
    okay("Client socket created successfully");

    //Prepare the server address structure
    serverAddress.sin_addr.s_addr = inet_addr(ip);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    //Connect to remote serverAddress
    if (connect(clientSocket , (struct sockaddr *)&serverAddress , sizeof(serverAddress)) < 0)
    {
        error("Client could not connect to server");
        return 1;
    }

    okay("Client connected successfully");

    while (true) {
        main_thread();
        if(quit == 1) break;
    }
    exit_chat();
}

void main_thread() {
    int option = display_menu_option();
    switch (option){
        case INSC:
            register_user();
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
        case QUIT:
            quit = 1;
            break;
        default:
            break;
    }
}

void show_user_list() {
    sprintf(message, "lst");
    send(clientSocket , message , strlen(message) , 0);
    long count = recv(clientSocket , message , 1024 , 0);
    message[count] = 0;
    puts(message);
    printf("Appuyez pour continuer...");
    getchar();
}

void chat_with_friend() {
    sprintf(message, "chat");
    send(clientSocket , message , strlen(message) , 0);
    // prompt received
    long count = recv(clientSocket , message , 1024 , 0);
    message[count] = 0;
    read_stdin(message);
    sprintf(message, "%s", input);
    send(clientSocket , message , strlen(message) , 0);
    // Connected list received
    count = recv(clientSocket , message , 1024 , 0);
    message[count] = 0;
    if(strcmp(message, "no_user") == 0) {
        sprintf(message, "User <%s> not found.", input);
        puts(message);
        return;
    }
    if(strcmp(message, "no_connected") == 0) {
        puts("Pas d'utilisateur connectés.");
        return;
    }
    // prompt for entering friend id
    puts(message);
    read_stdin("Entrez l'ID de votre ami : ");
    sprintf(message, "%s", input);
    send(clientSocket , message , strlen(message) , 0);
    recv(clientSocket , message , 1024 , 0);
    puts(message);
    bool chating = true;
    while (chating) {
        read_stdin("$@ > ");
        chating = strcmp(input, "end") != 0;
        send(clientSocket , input , strlen(input) , 0);
    }
}

void send_message() {
    while (true) {
        ssize_t count = read_stdin("> ");
        sprintf(message, "%s", input);
        if (count > 0) {
            if(strcmp(input, "exit") == 0) {
                quit = 1;
                break;
            }
            send(clientSocket , message , strlen(message) , 0);
        }
    }
    exit_chat();
}

void register_user() {
    sprintf(message, "ins");
    send(clientSocket , message , strlen(message) , 0);
    long count = recv(clientSocket , message , 1024 , 0);
    message[count] = 0;
    read_stdin(message);
    sprintf(message, "%s", input);
    send(clientSocket , input , strlen(input) , 0);
    count = recv(clientSocket , message , 1024 , 0);
    message[count] = 0;
    puts(message);
    puts("Inscription terminée.");
}

void exit_chat() {
    if(quit == 0) return;
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

void waiting() {
    puts("Appuyez une touche pour continuer...");
}

int display_menu_option() {
    int ch;
    while (1)
    {
        // present the menu, accept the user's choice
        printf("\n\nMenu des options :");
        printf("\n    I/i) Créer un nouveau compte (Inscription)."); //
        printf("\n    S/s) Envoyer un message a un ami.");
        printf("\n    C/c) Discuter avec un ami...");
        printf("\n    R/r) Lire mes messages.");
        printf("\n    L/l) Liste des comptes.");
        printf("\n    Q/q) Quitter le menu.");
        read_stdin("\n\nEntrer votre choix : ");
        ch = tolower(*input);
        //printf("%d", ch);
        if (!(ch == INSC || ch == LST || ch == CHAT || ch == SEND || ch == READ || ch == QUIT))
            continue;
        break;
    }
    //system("clear");
    return ch;
}
