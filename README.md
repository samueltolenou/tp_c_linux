﻿# tp_c_linux


créer un sever de messagerie tchat entre plusieurs clients 

//code serveur

1_inscription 
    generer id 
    stocker dans un fichier
    ouvrir le fichier "id" et recuperer le dernier id et incrementer et attribuer au nouveau client 
2_SEND
    un client doit pouvoir envoyer un message a un autre client
    lister tout les client inscrit sur le serveur 
    selectionner un client 
    etablir la connection avec les deux clients 
    envoie de message 
    mot cle end our arreter le service 

3_READ
un client doit pouvoir lire ses message grace a son id 


4_QUITTER
quitter une option 

----------------------------------------------------------------------------

//code client 

1_ le client fait la demande d'inscription a serveur et recois son id au format id qu'il stocke en saisissant son nom 

2_ le client fait la demande au server pour "emvoyer" un message a un client au serveur grace a id du second client 

3_ utilisateur lire ses messages | on affiche le contenu du fichier associer a id du client 

4_ quitter fin et arrêter
