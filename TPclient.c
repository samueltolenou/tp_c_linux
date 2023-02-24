#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h> 
 

int client_desc;
struct sockaddr_in addr;
char buffer[1024];


/*void Inscription(){
     do{
            printf("Bienvenu sur le processus d'inscription ");
            strcpy(buffer,"INS"); // copie INS dans le buffer
            send(client_desc, buffer, sizeof(buffer), 0); // envoie de INS au serveur
            read(client_desc, buffer, sizeof(buffer)); // serveur demande le pseudo
            printf("\nReponse du serveur : %s \n", buffer); //affiche le INS
            scanf("%s",buffer);  //on saisi le pseudo
            if (strcmp(buffer, "end")==0)
            break; 
            send(client_desc, buffer, sizeof(buffer), 0); //on envoie le pseudo saisi
            read(client_desc, buffer, sizeof(buffer)); // reception et lecture de ID*pseudo
            printf("Votre ID est : %s\n", buffer); // affichage du id*pseudo

        }while(strcmp(buffer, "end")!=0);  
}  */


void Inscription(){
                    int n,i;
                    printf("Bienvenu dans le processus d'inscription\n");
                    printf("entrer le nombre de client a inscrit\n");
                    scanf("%d",&n);
                    for(i=0;i<n;i++) {
                                sprintf(buffer,"INS");
                                send(client_desc, buffer, sizeof(buffer), 0); // envoie de INS au serveur
                                read(client_desc, buffer, sizeof(buffer)); // serveur demande le pseudo
                                printf("\nReponse du serveur : %s \n", buffer); //affiche le  message entrer votre pseudo
                                scanf("%s",buffer);  //on saisi le pseudo
                                if (strcmp(buffer, "end")==0)
                                break; 
                                send(client_desc, buffer, sizeof(buffer), 0); //on envoie le pseudo saisi
                                read(client_desc, buffer, sizeof(buffer)); // reception et lecture de ID
                                printf("Votre ID est: %s\n", buffer); // affichage du id
                                
                                read(client_desc, buffer, sizeof(buffer));
                                printf("votre id et nom:%s \n", buffer);
                                printf("Inscription terminer\n");
                            }
                }


void envoyer_message(int sock, int id_dest, char* message) {
    char buffer[1024];
    sprintf(buffer, "MSG %d %s", id_dest, message);
    send(client_desc, buffer, strlen(buffer), 0);
    }


void Send(){
    do{
        int eid;
        printf("envoyer un message a un client");
        strcpy(buffer,"SEND");
        send(client_desc, buffer, sizeof(buffer), 0);
        read(client_desc, buffer, sizeof(buffer));
        //scanf("%d",eid);
        sprintf(buffer,"%d",eid);
        send(client_desc, buffer, sizeof(buffer), 0); // envoi de id au serveur 

    }while(strcmp(buffer, "end")!=0);







}
void Read(){
    do{
        printf("lire ses message ");
        sprintf(buffer,"READ");
        send(client_desc, buffer, sizeof(buffer), 0); //envoi read au serveur 
        read(client_desc, buffer, sizeof(buffer));   // lire la reponse du serveur 

    }while(strcmp(buffer, "end")!=0);
}
void QUITTER(){

    strcpy(buffer,"end");
    send(client_desc, buffer, sizeof(buffer), 0);
    printf("vous quittez le programme👋!\n");
    exit(0);
    

}

int choix;

void menu(){
    do{
        printf("\t ⁕‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾⁕ \n"); 
        printf("\t︳                         Entrer votre choix:                          ׀ \n");
        printf("\t︳1— 👉 INSCRIPTION                                                     ׀ \n");
        printf("\t︳2— 👉 SEND                                                            ׀ \n");
        printf("\t︳3— 👉 READ                                                            ׀ \n");
        printf("\t︳4— 👉 QUITTER                                                         ׀ \n");
        printf("\t ⁕___________________________________________________________________⁕  \n");
        scanf("%d",&choix);
    }while(choix<1 || choix>4);
    system("clear");
}

void choie(){
    switch(choix){
        case 1: 
        printf("1-INSCRIPTION\n");
        Inscription(); //discuter avec le serveur   lui demander de nous donner un id 
        break;
        case 2:
        printf("2-SEND\n");
        Send();
        break;
        case 3:
        printf("3-READ\n");
        Read();
        break;
        case 4:
        printf("4-Vous quittez!\n");
        QUITTER();
        break;
    }  
}

int main(int argc, char **argv){

    // Creation socket
    client_desc = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket client créé avec succes \n");

    // Connection au server
    memset(&addr, '\n', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client_desc, (struct sockaddr*)&addr, sizeof(addr));
    printf("Connexion au serveur avec succes \n");
    menu();
    choie();

    //Fermeture socket
    close(client_desc);
    printf("\nClient Deconnecté 👋\n"); 
    
}
