#include <sys/types.h>   /* predefined types */
#include <unistd.h>      /* include unix standard library */
#include <arpa/inet.h>   /* IP addresses conversion utiliites */
#include <sys/socket.h>  /* socket library */
#include <stdio.h>	 /* include standard I/O library */
#include <errno.h>	 /* include error codes */
#include <string.h>	 /* include erroro strings definitions */
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>

#include "hash.h"

#define SIGKILL 9

struct ping_protocol{
    char name;
    int rec_port;
    clock_t lastPing;
    int flag;
};

struct Transaction{
  int  fd;
  char id;
  int  port;
  int  stateP;
};

void * trackerConnect(void *);
pthread_t thread_peer, thread_action,thread_receive, thread_menu;

pthread_mutex_t mutex_peer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_choice= PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_init(&mutex_peer,NULL);
int indexC = 0;
int Saldo = 100;
struct Transaction channels[10];
int sockudp, n, nwrite, nread,i,socktcp, size_peer,listenfd,connfd;
struct Transaction reachedPeer[5];
int IN_PAUSE,key;
char recvline[1025];
char recvline2[1025];
struct sockaddr_in servaddr,peer_list[10];
int control=1,port;
struct ping_protocol Pproto,ArrayPeers[10];

/*=========================================================
Ho aggiunto la memorizzazione dei peer con cui si è stabilita
una connessione.
===========================================================*/

void *trackerConnect(void * arg){

  while(1){

    pthread_mutex_lock(&mutex_peer);
    sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));
  //  printf("ENTRO NEL THREAD\n");
    if(Pproto.lastPing==0){
      //Sono un nuovo peer
      //Niente

      printf("MANDO SEGNALE AL TRACKER\n");
      recvfrom(sockudp,&Pproto,sizeof(struct ping_protocol),0,NULL,NULL);
      printf("RICEVUTO clock aggiornato\n");
      }
      else if(Pproto.flag==1){
      /* Richiesta lista peer disponibili*/

    //  sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));

      recvfrom(sockudp,&size_peer,sizeof(int),0,NULL,NULL);  //riceve prima il size della lista
      recvfrom(sockudp,&ArrayPeers,sizeof(ArrayPeers),0,NULL,NULL); //e poi i peer
      printf("ricevo porte TRACKER\n");
      printf("LISTA PEERS\n");
      for(i=0;i<size_peer;i++){
        printf("NOME = %c PORTA = %d\n",ArrayPeers[i].name, ArrayPeers[i].rec_port );}
      Pproto.flag=0;

    }else{
      //sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));
      recvfrom(sockudp,&Pproto,sizeof(struct ping_protocol),0,NULL,NULL);
    }
    pthread_mutex_unlock(&mutex_peer);
    sleep(2);
  }
  return 0;

}


void *peerConnect(void* arg){


    int porta;
    int amount;
    in_port_t porta_request;
    struct sockaddr_in toPeer;
    int connfd;

    if ( (socktcp = socket(AF_INET, SOCK_STREAM,0)) < 0) {
      fprintf(stderr,"socket error");
      exit (1);
    }

    pthread_mutex_lock(&mutex_peer);
    printf("Inserisci la porta del peer al quale vuoi connetterti\n");
    scanf("%d",&porta);
    printf("Quanto vuoi impegnare? (amount>0)\n");
    scanf("%d",&amount);

    toPeer=servaddr;
    porta_request=htons(porta);
    toPeer.sin_port=porta_request;

    if (connect(socktcp,(struct sockaddr *) &toPeer,sizeof(toPeer))<0){
      perror("connect\n");
      pthread_mutex_unlock(&mutex_peer);
    }

    snprintf(recvline,sizeof(recvline),"Ciao sono il peer %c e vorrei connettermi\n", Pproto.name);
    int lung;
    lung=strlen(recvline);
    write(socktcp,&lung,sizeof(int));
    write(socktcp,recvline,strlen(recvline));

    //Ora verifichiamo l'accettaziomne
    int verify;
    char idPeerConn;
    read(socktcp,&verify,sizeof(int));
    if(verify==4){
        printf("IL PEER HA RIFIUTATO LA CONNESSIONE\n" );
    }else if(verify==3){
        printf("CONNESSIONE ACCETTATA\n" );
        if(amount<=Saldo)  Saldo = Saldo - amount;
        snprintf(recvline,sizeof(recvline),"Ci siamo collegati sulla mia porta %d \n", Pproto.rec_port);
        lung=strlen(recvline);
        write(socktcp,&lung,sizeof(int));
        write(socktcp,recvline,strlen(recvline));
        read(socktcp,&idPeerConn,sizeof(int));
        //Inserire il peer nella propria lista dei peer
        channels[indexC].fd = socktcp;
        channels[indexC].id = idPeerConn;
        channels[indexC].port   = porta;
        channels[indexC].stateP = amount;
        printf(" FD = %d\n ID = %c\n PORTA = %d\n STATO = %d\n",channels[indexC].fd,channels[indexC].id,channels[indexC].port,channels[indexC].stateP);
        indexC++;

        }
        pthread_mutex_unlock(&mutex_peer);
    return 0;
}

void *openPort(void* arg){

  int choice;

  if ( ( listenfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in recAddr;
  recAddr=servaddr;
  recAddr.sin_port=htons(Pproto.rec_port);

  if ( bind(listenfd, (struct sockaddr *) &recAddr, sizeof(recAddr)) < 0 ) {
    perror("bind");
    exit(1);
  }

  if ( listen(listenfd, 1024) < 0 ) {
    perror("listen");
    exit(1);
  }
  while(1){
      if ( ( connfd = accept(listenfd, (struct sockaddr *) NULL, NULL) ) < 0 ) {
         perror("accept");
         exit(1);
      }
        printf("C'È UN TENTATIVO DI CONNESSIONE !!\n");
        //pthread_kill(thread_menu,SIGKILL);
        pthread_cancel(thread_menu);
        pthread_mutex_lock(&mutex_choice);

        IN_PAUSE=1;
        int amount1;
        int lungh;
        read(connfd,&lungh,sizeof(int));
        read(connfd,recvline2,lungh*sizeof(char));
        printf("%s\n",recvline2 );
        printf("ACCETTI LA CONNESSIONE?\n 3) Accetto \n 4)Rifiuti\n");
        scanf("%d",&choice);
        if(choice==3){
              printf("Quanto vuoi impegnare? (amount>=0)\n");
              scanf("%d",&amount1);
              if(amount1<=Saldo)  Saldo = Saldo - amount1;
              write(connfd,&choice,sizeof(int));
              read(connfd,&lungh,sizeof(int));
              read(connfd,recvline2,lungh*sizeof(char));
              write(connfd,&Pproto.name,sizeof(char));
              printf("%s \n",recvline2);

            }else if(choice==4){

                    write(connfd,&choice,sizeof(int));
                    close(connfd);
                    printf("Chiuso il canale\n" );

              }

          pthread_mutex_unlock(&mutex_choice);
          IN_PAUSE=0;

    }

    exit(0);
}

void *menu_exec(void* arg){


  printf("PREMI 1 PER COLLEGARTI E 2 per la lista\n" );
  scanf("%d",&key);
  if(key==1){
        fflush(stdin);
        pthread_create(&thread_action,NULL,peerConnect,NULL);
        pthread_join(thread_action,NULL);

  }else if (key==2){

      fflush(stdin);
      printf("Ecco la lista dei peer\n");
      Pproto.flag=1;
    }

return 0;
}
int main(int argc, char **argv)
{

  char sendbuff[4096],recvbuff[4096];
  fd_set fset;
  int fdInt[10000];


  if (argc != 2) {
    fprintf(stderr,"usage: %s <IPaddress>\n",argv[0]);
    exit (1);
  }
  if ( (sockudp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    fprintf(stderr,"socket error\n");
    exit (1);
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(1024);
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0){
    fprintf(stderr,"inet_pton error for %s\n", argv[1]);
    exit (1);
  }

  printf("INSERIRE NOME PEER\n");
  scanf("%c",&Pproto.name);
  printf("QUALE PORTA USI PER RICEVERE?\n");
  scanf("%d",&Pproto.rec_port);
  Pproto.lastPing=0;
  Pproto.flag=0;


  pthread_create(&thread_peer,NULL,trackerConnect,NULL);
  pthread_create(&thread_receive,NULL,openPort,NULL);

  //int k;

  while(1){

    pthread_mutex_lock(&mutex_choice);
    pthread_create(&thread_menu,NULL,menu_exec,NULL);
    pthread_join(thread_menu,NULL);
    pthread_mutex_unlock(&mutex_choice);
    sleep(2);

  }
pthread_join(thread_receive,NULL);
pthread_join(thread_peer,NULL);

exit(0);

}

/*
void menu(){
  int scelta;
  printf("Scegli azione:\n", );
  printf("1)Visualizza lista peer\n2)Connetti ad un thread\n",);
  scanf("d\n",&scelta);
  switch (scelta) {
    case 1: ;break;
    //case 2:
    default : printf("errore inserimento"\n", );
  }

}
*/
