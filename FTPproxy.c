#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>


#define SERVADDR "127.0.0.1"          // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille du tampon de demande de connexion
#define MAXBUFFERLEN 1024
#define MAXHOSTLEN 64
#define MAXPORTLEN 6

int connect2Server(const char *serverName, const char *port, int *descSock);
int convertirPort(const char *messagePort, int *socketACreer);
int gestionMultiClient(int ecode, int descSockCOM, char *buffer);
int deconnection(int descSockCOM, int sockServerCtrl, char *buffer);
bool verification_commande(char *commande);


int main(){
    pid_t pid;                       // Code retour d'un fork
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les 
                                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initailisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par 
                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);
     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran

     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, 
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);
     // Definition de la taille du tampon contenant les demandes de connexion

     while(1){
        
         ecode = listen(descSockRDV, LISTENLEN);
         if (ecode == -1) {
             perror("Erreur initialisation buffer d'écoute");
             exit(5);
         }
            len = sizeof(struct sockaddr_storage);
             // Attente connexion du client
             // Lorsque demande de connexion, creation d'une socket de communication avec le client
             printf("attente\n");
             descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
             if (descSockCOM == -1){
                 perror("Erreur accept\n");
                 exit(6);
             }
             pid = fork();
             switch(pid){
                case 0:
                 gestionMultiClient(ecode,descSockCOM,buffer);
                 exit(1);
             }


     }

  //Fermeture de la connexion
  close(descSockCOM);
  close(descSockRDV);

}

int gestionMultiClient(int ecode,int descSockCOM, char *buffer){
    

        printf("Nouvelle connexion\n\n");
        // Echange de données avec le client connecté
        strcpy(buffer, "220 BLABLABLA\n");
        write(descSockCOM, buffer, strlen(buffer));

        //Echange de donneés avec le serveur
        ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("MESSAGE RECU DU CLIENT: \"%s\".\n",buffer);

    
        char login[50], serverName[50];

        sscanf(buffer, "%49[^@]@%50s", login, serverName);
        strcat(login,"\n");

        printf("login est %s et la machine est %s\n", login, serverName);

        int sockServerCtrl;

        //Connection de la socket Serveur Controle au serveur ftp
        ecode = connect2Server(serverName,"21",&sockServerCtrl);
        if (ecode == -1){
            perror("Pb connexion avec le serveur FTP");
            exit(7);
        }

        //S->P : Lire 220 ...
        ecode = read(sockServerCtrl, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);

        //P->S Envoyer USER ... au serveur
        write(sockServerCtrl, login, strlen(login));

        //S->P : Lire 331
        ecode = read(sockServerCtrl, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);

        //P->C : 331
        printf("Envoie de 331\n");
        write(descSockCOM, buffer, strlen(buffer));

        //C->P
        ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("Mot de passe envoyé: \"%s\".\n",buffer);

        //envoie du mdp P->S
        write(sockServerCtrl,buffer,strlen(buffer));

        // réception du message après identification MDP S-> P
        ecode = read(sockServerCtrl, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);

        //transmission du message de connexion au client P->C
        write(descSockCOM, buffer, strlen(buffer));

        // recuperation de SYST et transmission vers le proxy 
        ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("MESSAGE RECU DU CLIENT: \"%s\".\n",buffer);
        //transmission au serveur 
        write(sockServerCtrl, buffer, strlen(buffer));

        // réception du message SYST S-> P
        ecode = read(sockServerCtrl, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);

        write(descSockCOM,buffer,strlen(buffer));
    
        //recuperation du PORT
        ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
        if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
        buffer[ecode] = '\0';
        printf("MESSAGE RECU DU CLIENT: \"%s\".\n",buffer);

        
        while(1){

            if(verification_commande(buffer)){  //Vérification que la commande entrée est supportée par la version actuelle 

                // Vérification si la commande passée est QUIT  
                if(strstr(buffer,"QUIT") != NULL){
                    deconnection(descSockCOM,sockServerCtrl,buffer);
                }

                int socketActiveCP;

                // Transformation du message PORT en une socket à traver la fonction convertir port qui appelle ensuite connect2server
                ecode = convertirPort(buffer,&socketActiveCP);
                if (ecode != 0){
                perror("Pb connexion avec la conversion de PORT");
                exit(7);
                }

                // Demande de passage en mode actif au serveur FTP
                write(sockServerCtrl,"PASV\n\r",strlen("PASV\n\r"));

                // Reception de 227
                ecode = read(sockServerCtrl, buffer, MAXBUFFERLEN);
                if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
                buffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);

                int socketPassiveSP;

                // Conversion de l'adresse renvoyé par 227 en une socket active
                ecode=convertirPort(buffer,&socketPassiveSP);
                if (ecode != 0){
                perror("Pb connexion avec la conversion de PORT");
                exit(7);
                }

                printf("Connexion avec résultat PASV OK\n");


                // //Envoie de 200 au client
                write(descSockCOM,"200\r\n",strlen("200\r\n"));

                //Ecoute de LIST et renvoie vers le serveur
                ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
                if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
                buffer[ecode] = '\0';
                printf("MESSAGE RECU DU CLIENT: \"%s\".\n",buffer);

                write(sockServerCtrl,buffer,strlen(buffer));

                //Ecoute de réponse LIST S->P
                ecode = read(sockServerCtrl, buffer, MAXBUFFERLEN);
                if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
                buffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);

                write(descSockCOM,buffer,strlen(buffer));

                // printf("Envoie de LIST au serveur\n");
                // write(sockServerCtrl,"LIST\r\n",strlen("LIST\r\n"));

                ecode = read(socketPassiveSP, buffer, MAXBUFFERLEN);

                while(ecode != 0){
                    printf(buffer);
                    write(socketActiveCP,buffer,strlen(buffer));
                    ecode = read(socketPassiveSP, buffer, MAXBUFFERLEN);
                    if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
                    buffer[ecode] = '\0';
                }

                close(socketActiveCP);
                close(socketPassiveSP);

                // Ecoute du message fin de transmission S->P
                ecode = read(sockServerCtrl,buffer,MAXBUFFERLEN);
                if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
                buffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer); 

                // Transmission 226 au client
                write(descSockCOM,buffer,strlen(buffer));       

                // Ecoute de la prochaine commande
                ecode = read(descSockCOM,buffer,MAXBUFFERLEN);
                if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
                buffer[ecode] = '\0';
                printf("MESSAGE RECU DU CLIENT  ICI: \"%s\".\n",buffer);
                }

                else{

                    char mess_erreur[] = "502 COMMANDE NON GERE (uniquement ls ou exit) \n\r";
                    write(descSockCOM,mess_erreur,strlen(mess_erreur));
                   
                    // Ecoute de la prochaine commande
                    ecode = read(descSockCOM,buffer,MAXBUFFERLEN);
                    if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
                    buffer[ecode] = '\0';
                    printf("MESSAGE RECU DU CLIENT  ICI: \"%s\".\n",buffer);
                }
            }

            //     ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
            //     if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
            //     buffer[ecode] = '\0';
            //     printf("MESSAGE RECU DU CLIENT: \"%s\".\n",buffer);
            // }

            

}

int convertirPort(const char *messagePort, int *socketACreer){
    int a,b,c,d,e,f,g,ecode,code_int;
    char adresseIP[20],port[15], code[3];

    char *result = strstr(messagePort,"227 Entering");

    if ( result == NULL ) {

        printf("Calcul de l'adresse pour PORT\n");
        sscanf(messagePort,"PORT %d,%d,%d,%d,%d,%d",&a,&b,&c,&d,&e,&f);
        sprintf(adresseIP,"%d.%d.%d.%d",a,b,c,d);

        g = (e*256)+f;
        sprintf(port,"%d",g);

        printf("Adresse et Port calcules par la fonction convertirPort %s,%s\n\n", adresseIP,port);

        ecode = connect2Server(adresseIP,port,socketACreer);
        if (ecode == -1){
            perror("Pb connexion avec le serveur FTP");
            exit(7);
        }

    } else {

        printf("Calcul de l'adresse pour 227\n");
        sscanf(messagePort,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&a,&b,&c,&d,&e,&f);
        sprintf(adresseIP,"%d.%d.%d.%d",a,b,c,d);
        g = (e*256)+f;
        sprintf(port,"%d",g);

        printf("Adresse et Port calcules par la fonction convertirPort %s,%s\n\n", adresseIP,port);

        ecode = connect2Server(adresseIP,port,socketACreer);
        if (ecode == -1){
            perror("Pb connexion avec le serveur FTP");
            return(-1);
        }

        return 0; 
    }
    
}

int connect2Server(const char *serverName, const char *port, int *descSock){

    int ecode;                     // Retour des fonctions
    struct addrinfo *res,*resPtr;  // Résultat de la fonction getaddrinfo
    struct addrinfo hints;
    
    bool isConnected = false;      // booléen indiquant que l'on est bien connecté

    // Initailisation de hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_UNSPEC;      // les adresses IPv4 et IPv6 seront présentées par 
                                          // la fonction getaddrinfo

    //Récupération des informations sur le serveur
    ecode = getaddrinfo(serverName,port,&hints,&res);
    if (ecode){
        fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
        return -1;
    }

    resPtr = res;

/*
    int val;    
    valeur -> val
    adresse de val -> &val

    int *vaux;
    valeur -> *vaux
    adresse -> vaux
*/
    while(!isConnected && resPtr!=NULL){

        //Création de la socket IPv4/TCP
        *descSock = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
        if (*descSock == -1) {
            perror("Erreur creation socket");
            return -1;
        }
  
        //Connexion au serveur
        ecode = connect(*descSock, resPtr->ai_addr, resPtr->ai_addrlen);
        if (ecode == -1) {
            resPtr = resPtr->ai_next;           
            close(*descSock);   
        }
        // On a pu se connecté
        else isConnected = true;
    }
    freeaddrinfo(res);
    if (!isConnected){
        perror("Connexion impossible");
        return -1;
    }

    return 0;
}

int deconnection(int descSockCOM, int sockServerCtrl, char *buffer){

    printf("On quitte\n");
    write(sockServerCtrl,buffer,strlen(buffer));
    
    read(sockServerCtrl,buffer,MAXBUFFERLEN);
    printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);

    write(descSockCOM,buffer,strlen(buffer));
    exit(1);

}


bool verification_commande(char *commande){
    char check_commande[5];
    char list[]="PORT", quit[]="QUIT";
    
    strncpy(check_commande,commande,4);
    printf("Les 4 premiers car sont : %s\n",check_commande);
    if(strcmp(check_commande,list) != 0 && strcmp(check_commande,quit) != 0){
        printf("Je retourne faux\n");
        strcpy(check_commande,"\0");
        return false;
    } 
    strcpy(check_commande,"\0");
    return true;
}
