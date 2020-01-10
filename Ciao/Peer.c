#include <arpa/inet.h> /* IP addresses conversion utiliites */
#include <errno.h>     /* include error codes */
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h> /* include standard I/O library */
#include <stdlib.h>
#include <string.h> /* include erroro strings definitions */
#include <sys/select.h>
#include <sys/socket.h> /* socket library */
#include <sys/types.h>  /* predefined types */
#include <time.h>
#include <unistd.h> /* include unix standard library */

#include "lista.h"
#define SIGKILL 9

struct ping_protocol {
  char name;
  int rec_port;
  clock_t lastPing;
  int flag;
};

struct Transaction {
  int fd;
  char id;
  int port;
  int stateP;
};

void *trackerConnect(void *);
void* menu_exec(void *);
pthread_t thread_peer, thread_action, thread_receive, thread_menu, thread_set, thread_channel;

int fdApp;

pthread_mutex_t mutex_peer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_choice = PTHREAD_MUTEX_INITIALIZER;  //UTILIZZATO IN TUTTE LE FUNZIONI APPARTENENTI AL MENU PER SINCORNIZZARE L'ESECUZIONE DELLO STESSO
pthread_mutex_t mutex_channel = PTHREAD_MUTEX_INITIALIZER;


int indexC = 0;
int Saldo = 100;
//struct Transaction channels[5];
int sockudp, n, nwrite, nread, i, socktcp, size_peer, listenfd, connfd;
// struct Transaction reachedPeer[5];
int IN_PAUSE, key;
char recvline[1025];
char recvline2[1025];
struct sockaddr_in servaddr, peer_list[10];
int control = 1, port;
struct ping_protocol Pproto;
struct ping_protocol *ArrayPeers;
fd_set fset;

//void sendMoney(void *);
/*=========================================================
Ho aggiunto la memorizzazione dei peer con cui si è stabilita
una connessione.
===========================================================*/

// GESTIONE RICHIESTE DI CONNESSIONE IN ENTRATA

void *peer_set(void *arg) {

  IN_PAUSE = 1;
  int amount1;
  int lungh;
  int choice;

  int fd = fdApp;

  pthread_cancel(thread_menu);
  read(fd, &lungh, sizeof(int));
  read(fd, recvline2, lungh * sizeof(char));
  printf("%s \n", recvline2);
  printf("ACCETTI LA CONNESSIONE?\n 3) Accetto \n 4)Rifiuti\n");
  scanf("%d", &choice);

  if (choice == 3) {
    FD_SET(fd, &fset);
    printf("Quanto vuoi impegnare? (amount>=0)\n");
    scanf("%d", &amount1);
    if (amount1 <= Saldo)
      Saldo = Saldo - amount1;

    write(fd, &choice, sizeof(int));
    read(fd, &lungh, sizeof(int));
    read(fd, recvline2, lungh * sizeof(char));
    write(fd, &Pproto.name, sizeof(char));
    printf("%s \n", recvline2);


    TRANSACTION app2;
    app2.fd = fd;
    char id;
    read(fd, &id, sizeof(char)); // ricevo ID interlocutore
    app2.id = id;
    int port;
    read(fd, &port, sizeof(int)); // ricevo porta interlocutore
    app2.port = port;
    app2.stateP = amount1;
    insertChannel(app2);
    printChannels();
    indexC++;

  } else if (choice == 4) {
    write(connfd, &choice, sizeof(int));
    close(connfd);
    printf("Chiuso il canale\n");
  }
  printf("UNLOCK IN PEER_SET\n");
  pthread_mutex_unlock(&mutex_choice);
  IN_PAUSE = 0;

  return 0;
}
void *trackerConnect(void *arg) {

  while (1) {

    //pthread_mutex_lock(&mutex_peer);

    sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0,
           (struct sockaddr *)&servaddr, sizeof(servaddr));

    //  printf("ENTRO NEL THREAD\n");
    if (Pproto.lastPing == 0) {
      // Sono un nuovo peer
      printf("MANDO SEGNALE AL TRACKER\n");
      recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);
      printf("RICEVUTO clock aggiornato\n");
    } else if (Pproto.flag == 1) {
      /* Richiesta lista peer disponibili*/
      //  sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0, (struct
      //  sockaddr *) &servaddr, sizeof(servaddr));
      recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL,
               NULL); // riceve prima il size della lista
               printf("SIZEPEER=: %d\n",size_peer);

               free(ArrayPeers);
      ArrayPeers=(struct ping_protocol*)malloc(size_peer * sizeof(struct ping_protocol));

      recvfrom(sockudp, ArrayPeers, size_peer*sizeof(struct ping_protocol), 0, NULL,
               NULL); // e poi i peer  direttamente dalla hash conenuta nel traker che essendo già un puntatore ad una struct
               //non necessita di un indirizzamento

      printf("ricevo porte TRACKER\n");
      printf("LISTA PEERS\n");

      for (i = 0; i < size_peer; i++) {

          printf("Porta= %d Ping = %ld\n", ArrayPeers[i].rec_port,
                 ArrayPeers[i].lastPing);
        }
        Pproto.flag = 0;
        printf("UNLOCK IN TRACKER CONNECT\n");
        pthread_mutex_unlock(&mutex_choice);
        free(ArrayPeers);


      //display();

    } else // flag=0 cioè ping
    {

      recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);
    }
    //pthread_mutex_unlock(&mutex_peer);
    sleep(2);
  }
  return 0;
}
void *peerConnect(void *arg) {

  int porta;
  int amount;
  in_port_t porta_request;
  struct sockaddr_in toPeer;
  int connfd;

  if ((socktcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket error");
    exit(1);
  }

  printf("Inserisci la porta del peer al quale vuoi connetterti\n");
  scanf("%d", &porta);
  printf("Quanto vuoi impegnare? (amount>0)\n");
  scanf("%d", &amount);

  toPeer = servaddr;
  porta_request = htons(porta);
  toPeer.sin_port = porta_request;

  if (connect(socktcp, (struct sockaddr *)&toPeer, sizeof(toPeer)) < 0) {
    perror("connect\n");
  }

  // HO USATO ALT COME MONETA IN ONORE DEL MAGICO
  snprintf(recvline, sizeof(recvline),
           "Ciao sono il peer %c e vorrei connettermi con %d ALT\n",
           Pproto.name, amount);
  int lung;
  lung = strlen(recvline);

  write(socktcp, &lung, sizeof(int));
  write(socktcp, recvline, strlen(recvline));

  // Ora verifichiamo l'accettazione
  int verify;
  char idPeerConn;
  read(socktcp, &verify, sizeof(int));
  if (verify == 4) {
    printf("IL PEER HA RIFIUTATO LA CONNESSIONE\n");

  } else if (verify == 3) {
    printf("CONNESSIONE ACCETTATA\n");
    if (amount <= Saldo)
      Saldo = Saldo - amount;
    snprintf(recvline, sizeof(recvline),
             "Ci siamo collegati sulla mia porta %d \n", Pproto.rec_port);
    lung = strlen(recvline);
    write(socktcp, &lung, sizeof(int));
    write(socktcp, recvline, strlen(recvline));
    read(socktcp, &idPeerConn, sizeof(char));
    write(socktcp, &Pproto.name, sizeof(char));
    write(socktcp, &Pproto.rec_port, sizeof(int));
    // Inserire il peer nella propria lista dei peer
    if (indexC < 5) {

      TRANSACTION app;
      app.fd = socktcp;
      app.id = idPeerConn;
      app.port = porta;
      app.stateP = amount;

      insertChannel(app);
      printChannels();
      indexC++;
    }
  }
  printf("UNLOCK IN PEER_CONNECT\n");
  pthread_mutex_unlock(&mutex_choice);
  return 0;
}
void *openPort(void *arg) {

  int choice;

  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in recAddr, peerAddr;
  recAddr = servaddr;
  recAddr.sin_port = htons(Pproto.rec_port);

  if (bind(listenfd, (struct sockaddr *)&recAddr, sizeof(recAddr)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(listenfd, 1024) < 0) {
    perror("listen");
    exit(1);
  }

  while (1) {

    int connectfd;

    if ((connectfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
      perror("accept");
      exit(1);
    }
    printf("C'È UN TENTATIVO DI CONNESSIONE !!\n");
    //abort(menu_exec);
    fdApp = connectfd;
    pthread_create(&thread_set, NULL, peer_set, NULL);
    pthread_join(thread_set, NULL);
  }

  exit(0);
}
void *channelConnect(void *arg){

    char keyC;
    int portascelta;
    fflush(stdin);
  //printChannelsList();
    printChannels();
    printf("Vuoi connetterti ad uno state channel? [s/n]\n");
    fflush(stdin);
    scanf("%s",&keyC);
    printf("Carattere: %c\n",keyC);

    if(keyC == 's'){
      fflush(stdin);
      printf("Inserisci la porta del peer al quale vuoi connetterti\n");
      scanf("%d",&portascelta);
    }else{
      printf("Afammocc\n");
    }
    pthread_mutex_unlock(&mutex_choice);
    printf("UNLOCK CHANNELCONNECT\n" );
    return 0;

}
void* menu_exec(void *arg) {



  printf("PREMI 1 PER COLLEGARTI E 2 per la lista ");
  if(indexC>0)
    printf(" o premi 3 per visualizzare gli state channels");

  printf("\n");
  scanf("%d", &key);
  switch (key) {
  case 1:
    fflush(stdin);
    pthread_create(&thread_action, NULL, peerConnect, NULL);
    pthread_join(thread_action, NULL);
    break;
  case 2:
    fflush(stdin);
    printf("Ecco la lista dei peer\n");
    Pproto.flag = 1;
    break;
  case 3:
    fflush(stdin);
    pthread_create(&thread_channel, NULL, channelConnect, NULL);
    pthread_join(thread_channel, NULL);
    printf("JOIN CHANNEL CONNECT\n");
    break;
  }


  return 0;
}

int main(int argc, char **argv) {
  ArrayPeers=(struct ping_protocol *)malloc(1 * sizeof(struct ping_protocol));
  initChannel();
  char sendbuff[4096], recvbuff[4096];

  if (argc != 2) {
    fprintf(stderr, "usage: %s <IPaddress>\n", argv[0]);
    exit(1);
  }
  if ((sockudp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    fprintf(stderr, "socket error\n");
    exit(1);
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(1024);
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
    fprintf(stderr, "inet_pton error for %s\n", argv[1]);
    exit(1);
  }

  printf("INSERIRE NOME PEER\n");
  scanf("%c", &Pproto.name);
  printf("QUALE PORTA USI PER RICEVERE?\n");
  scanf("%d", &Pproto.rec_port);
  Pproto.lastPing = 0;
  Pproto.flag = 0;


  pthread_create(&thread_peer, NULL, trackerConnect, NULL);
  pthread_create(&thread_receive, NULL, openPort, NULL);


  while (1) {

    pthread_mutex_lock(&mutex_choice);
    pthread_create(&thread_menu, NULL, menu_exec, NULL);
    pthread_join(thread_menu, NULL);

    printf("Fottuta sleep!\n");
    sleep(2);
  }
  pthread_join(thread_receive, NULL);
  pthread_join(thread_peer, NULL);

  exit(0);
}
/*
void sendMoney(void *arg){
  printf("A quale peer vuoi inviare denaro?(inserisci porta)\n");
  printPeerListpeerList();
  int x,fdX,sendALT=0; //x e' la porta, fdX e' il suo valore hash associato
nell'array channels scanf("%d",&x); fdX=hashcode(x);
  if(FD_ISSET(fdX,&fset)){//CONTROLLO SE E' ANCORA ATTIVA LA CONNESSIONE
    write(fdX,1,sizeof(int)); //invio codice per identficare la richiesta
d'invio denaro
    printf("Quanto vuoi inviare? Max:%d\n",channels[fdX].stateP );
    scanf("%d\n",&sendALT);
    if(sendALT>channels[fdX].stateP){
    }
    write(fdX,&sendALT,sizeof(int));
  }
}
*/
