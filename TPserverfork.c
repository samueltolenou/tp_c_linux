#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>


struct client{
    int id;
    char *nom;
};
typedef struct client client;

int ID=2023; 
int main(int argc, char **argv) {
    int server_desc, client_desc;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[1024];
    // Création socket
    server_desc = socket(AF_INET, SOCK_STREAM, 0);
    printf("[+]Creation du socket serveur.\n");

    // Préparation de l'adresse du serveur
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Liaison de l'adresse au socket
    bind(server_desc, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("[+]Bind to port %s\n", argv[1]);

    // Ecoute des connexions
    listen(server_desc, 4);
    printf("[...]En attente de connexion\n");

    while(1) {
        // Acceptation de la connexion
        client_len = sizeof(client_addr);
        client_desc = accept(server_desc, (struct sockaddr*)&client_addr, &client_len);
        printf("[+]Connexion de %s:%d accepter.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
       

        // Création d'un processus enfant pour traiter la connexion
        if (fork() == 0) {
            close(server_desc);
            while(1) {
                //Réception des données
                recv(client_desc, buffer, 1024, 0);
                printf("Message recu: %s\n", buffer);

                if(strcmp(buffer, "INS") == 0){
					//printf("[+]Demande d'inscription de %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    printf("[+]Demande d'inscription\n");
                    sprintf(buffer,"entrer votre pseudo"); //copie de entrer votre pseudo dans buffer
                    send(client_desc, buffer, strlen(buffer), 0); //envoi du message entrer votre pseudo
                    recv(client_desc, buffer, 1024, 0); // Lecture du pseudo 
                    client cli;
                  //  sprintf(cli.nom,"%s",buffer);
                    //cli.nom[10]=buffer;
//avant de donner un id il faut ouvrir le fichier et recuperer le dernier id ensuite converti et incremente
                    FILE* li = NULL;
                    char contenu[6];
                    li=fopen("id.txt","r");
                    while (fgets(contenu,6,li) != NULL){ 
                    }
                    ID=atoi(contenu);
                    ID++;
                    printf("id du client:%d\n",ID); 
                    fclose(li);
                    sprintf(buffer,"%d",ID);
                    send(client_desc, buffer, strlen(buffer), 0); //envoi au client son id
                    li=fopen("id.txt","a+");
                    fprintf(li,"%d\n", ID);
                    fclose(li);
                  //  sprintf(buffer,"%d*%s",ID,cli.nom);
                  //  printf("%s",buffer);
                   // send(client_desc, buffer, strlen(buffer), 0); //envoie de id*nom au client 
                
                    
                }
                if(strcmp(buffer, "SEND") == 0){
                    printf("Bienvenu sur l'interface d'envoi\n");
                    printf("voici les commandes disponibles:\n liste\n envoyer\n");
                    //comment verifier si le client n'est pas en ligne et s'il n'ai en ligne on stocke le contenu dans un fichier
                    sprintf(buffer,"entrer id  du client que vous voulez ecrire");
                    send(client_desc, buffer, strlen(buffer), 0);
                    recv(client_desc, buffer, 1024, 0);  // recois id saisi 
                    /*il faut que le serveur verifie si l id de l'autre client est en ligne si oui il lui envoie les messages 
                    sinon il les stocke dans un fichier avec l'id du client  qu'on lira apres
                    mais avant toutes choses il faut lister les id des client et ensuite fait la selection
                    
                    
                    
                    */




                    
					


                }

                if(strcmp(buffer, "READ") == 0){
                    printf("[+]Lecture de vos messages");
                    
                    //chercher le fichier qui porte  l id du client et afficher son contenu 

                }
                if(strcmp(buffer, "end") == 0){
                    printf("le client à quitter le programme👋!\n");
                    break;
                }
                
            }
            close(client_desc);
            exit(0);
        }
        close(client_desc);
    }
    return 0;
}