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
#include <sys/time.h>

#include "lista.h"
#define SIGKILL 9
#define FALSE 0
#define TRUE 1

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

struct floodPack {
  int dest_port;
  int n_hops;
  int hops[5];
  int saldoT;
  int reached; //controllo se e' arrivato a destinazione
};




void *trackerConnect(void *);
void *channelConnect(void *);
void *menu_exec(void *);
void *Gestione(void  *) ;
pthread_t thread_peer, thread_action, thread_receive, thread_menu, thread_set, thread_channel,thread_gestione;

pthread_mutex_t mutex_choice = PTHREAD_MUTEX_INITIALIZER;  //UTILIZZATO IN TUTTE LE FUNZIONI APPARTENENTI AL MENU PER SINCORNIZZARE L'ESECUZIONE DELLO STESSO
pthread_mutex_t mutex_controllo= PTHREAD_MUTEX_INITIALIZER; //nel caso in cui mando flag=2 nella peer connect ho bisogno di un lock per evitare un controllo a vuoto
pthread_mutex_t mutex_fset= PTHREAD_MUTEX_INITIALIZER;//mutex che ci garantisce in mutua esclusione l'inserimento e la canzellazione nell'array fset

int fdApp;
int indexC;
int Saldo = 100;
int sockudp, n, nwrite, nread, i, socktcp, size_peer, listenfd, connfd,maxfd;
int IN_PAUSE, key;
char recvline[1025];
char recvline2[1025];
struct sockaddr_in servaddr, peer_list[10];
int control = 1, port;
struct ping_protocol Pproto;
struct ping_protocol *ArrayPeers;
fd_set fsetmaster;
fd_set appFset;

//void sendMoney(void *);
/*

    BISOGNA VEDERE CHE COSA  È LA FUNCTRION epoll()  E I SUOI RELATIVI CAZZI

*/
// GESTIONE RICHIESTE DI CONNESSIONE IN ENTRATA
void *peerAccept(void *arg) {

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

  if (choice == 3) {/*OH OH teletype*/
    printf("Quanto vuoi impegnare? (amount>=0)\n");
    scanf("%d", &amount1);

    pthread_mutex_lock(&mutex_fset);
    FD_SET(fd,&fsetmaster);

    if (maxfd<fd){
      maxfd=fd;
    }
    pthread_mutex_unlock(&mutex_fset);

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
    read(fd, &port, sizeof(int)); // ricevo porta interlocutoreinsertChannel
    app2.port = port;
    app2.stateP = amount1;
    insertChannel(app2); //inserisco all'interno della mia lista di state channels
    //printChannels();
    indexC++;



  } else if (choice == 4) {
    write(connfd, &choice, sizeof(int));
    close(connfd);
    printf("Canale Chiuso\n");
  }
//  printf("UNLOCK IN peerAccept\n\n");
  pthread_mutex_unlock(&mutex_choice);
  IN_PAUSE = 0;

  return 0;
}

void *trackerConnect(void *arg) {

  //pthread_create(&thread_gestione,NULL,Gestione,NULL);

  while (1) {

    sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0,
           (struct sockaddr *)&servaddr, sizeof(servaddr));

           switch(Pproto.flag){

                      case 0:
                              if (Pproto.lastPing == 0) {
                                  // Sono un nuovo peer
                                  printf("MANDO SEGNALE AL TRACKER\n");
                                  recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);
                                  printf("RICEVUTO clock aggiornato\n");
                                }else{
                                      //PING SEMPLICE
                                      recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);

                                }
                                break;

                      case 1:
                                /* Richiesta lista peer disponibili*/
                                free(ArrayPeers);
                                recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL, NULL); // riceve prima il size dell'array
                                ArrayPeers=realloc(ArrayPeers,size_peer * sizeof(struct ping_protocol));
                                recvfrom(sockudp, ArrayPeers, size_peer*sizeof(struct ping_protocol), 0, NULL,NULL); // e poi i peer  direttamente dalla hash conenuta nel traker
                                                                                                                    //che essendo già un puntatore ad una struct
                                                                                                                      //non necessita di un indirizzamento
                                printf("\tLista Peers Disponibili\n");

                                for (i = 0; i < size_peer; i++) {

                                    printf("ID = %c Porta= %d\n", ArrayPeers[i].name, ArrayPeers[i].rec_port);
                                  }
                                  printf("\n");
                                  Pproto.flag = 0;
                                //  printf("UNLOCK IN TRACKER CONNECT\n\n");
                                  pthread_mutex_unlock(&mutex_choice);

                                  break;

                      case 2:
                                  free(ArrayPeers);
                                  recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL, NULL);
                                  ArrayPeers=realloc(ArrayPeers,size_peer * sizeof(struct ping_protocol));
                                  recvfrom(sockudp, ArrayPeers, size_peer*sizeof(struct ping_protocol), 0, NULL,NULL);
                                  Pproto.flag = 0;
                                  pthread_mutex_unlock(&mutex_controllo);
                                  break;

           }



          sleep(2);
  }
   //pthread_join(thread_gestione,NULL);
  return 0;
}

void *peerConnect(void *arg) {


  int porta;
  int amount=0;
  int j,indice=0;
  in_port_t porta_request;
  struct sockaddr_in toPeer;
  TRANSACTION* app4;
  int connfd,controllore=0;
  pthread_mutex_lock(&mutex_controllo);



  printf("Inserisci la porta del peer al quale vuoi connetterti\n");
  scanf("%d", &porta);

  app4=searchChannel(porta);




  Pproto.flag=2;
  pthread_mutex_lock(&mutex_controllo);
  printf("DOPO LOCK\n" );
  for(j=0;j<size_peer;j++){

      if (ArrayPeers[j].rec_port==porta){controllore=1;break;}  //NELL'ULTIMA CONNESSIONE AL TRAKER ESISTE QUELLA PORTA 1 controllo

  }

  printf("PRIMA DELL'IF\n" );
  if(controllore==1){
  //SE ESISTE QUESTO PEER
  if(app4!=NULL){  //2 controllo
//Se sono già connesso a questa porta in uno state channel
    pthread_create(&thread_channel, NULL, channelConnect, &porta);
    pthread_join(thread_channel, NULL);
    //printf("JOIN CHANNEL CONNECT\n");

    /*
      }else if (indexC!=0){

            TRANSACTION * appInter=channels->pnext;
            struct floodPack Fpack;

            printf("Quanto vuoi impegnare? (amount>0)\n");
            scanf("%d", &amount);

            Fpack.porta=porta;
            Fpack.n_hops=0;
            Fpack.hops[0]=Pproto.rec_port;
            Fpack.saldoT=amount;
            Fpack.reached=0;
    //SE QUESTO PEER PUO ESSERE RAGGIUNTO DA UN MIO STATE CHANNEL

            while(appInter!=NULL){

                      write(appInter.fd,&Fpack,sizeof(struct floodPack));
                      read(appInter.fd,&Fpack,sizeof(struct floodPack));
                    if(Fpack.reached==1){



                            break;

                  }



            }


      }if(Fpack.reached==0){
  */
}else{

  if ((socktcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket error");
    exit(1);
  }

  if(amount==0){
  printf("Quanto vuoi impegnare? (amount>0)\n");
  scanf("%d", &amount);
  }

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
    printf("\t=====CONNESSIONE RIFIUTATA=====\n");
    close(socktcp);

  } else if (verify == 3) {
    printf("\t=====CONNESSIONE ACCETTATA=====\n");
    printf("\n");

    pthread_mutex_lock(&mutex_fset);
    FD_SET(socktcp,&fsetmaster);

    if (maxfd<socktcp)
      maxfd=socktcp;

    pthread_mutex_unlock(&mutex_fset);




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
      //printChannels();
      indexC++;

      //pthread_create(&thread_gestione,NULL,Gestione,&socktcp);
    }
  }
}
//  printf("UNLOCK IN PEER_CONNECT\n\n");
}
  else{
      printf("PEER NON DISPONIBILE \n");
}

  pthread_mutex_unlock(&mutex_controllo);
  pthread_mutex_unlock(&mutex_choice);
  return 0;
}

void *Gestione(void  *arg) {


  printf("\n\nSONO IN GESTIONE\n\n" );


  //int *descriptor=(int *)arg;
  struct timeval timer;
  //timer.tv_sec=5;
  //timer.tv_usec=0;
  int o_select,indice;
  struct floodPack Fpack;

  while(1){


        memcpy(&appFset,&fsetmaster,sizeof(fsetmaster));
        timer.tv_sec=1;
        timer.tv_usec=0;
        o_select=select(maxfd+1,&appFset,NULL,NULL,&timer);

      //  printf("MAXFD %d\n",maxfd);

        if(o_select!=0)  {
          for(indice=listenfd+1;indice<=maxfd;indice++){

                if(FD_ISSET(indice,&appFset)){
                      printf("DENTRO L'ISSET\n");
                      read(indice,&Fpack,sizeof(struct floodPack));
                      printf("REACHED = %d\n",Fpack.reached );
                }
          }


        }
  }
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

maxfd=listenfd;


  while (1) {

    int connectfd;

    if ((connectfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
      perror("accept");
      exit(1);
    }

    printf("\n\t=====TENTATIVO DI CONNESSIONE=====\n\n");

    fdApp = connectfd;

    pthread_create(&thread_set, NULL, peerAccept, NULL);
    pthread_join(thread_set, NULL);
  }
  sleep(2);
  exit(0);
}

void *channelConnect(void *arg){

    int keyC, appFD;
    int control;
    char appID;


    int *portascelta=(int *)arg;
    TRANSACTION *app3=searchChannel(*portascelta);
    struct floodPack Fpack;
    printf("%d\n",*portascelta );
    printf("VUOI:\n 1)INVIARE ALT \n 2)CHIUDERE IL CANALE \n" );
    fflush(stdin);
    printf("SALDO SU CANALE = %d ALT \n",app3->stateP );
    scanf("%d",&keyC);
    switch (keyC) {
      case 1:
        Fpack.reached=1;
        printf("SALDO SU CANALE = %d ALT \n",app3->stateP );  //CONTROLLO != NULL VEMIVA GIA FATTO NELLA PEER CONNECT
        write(app3->fd,&Fpack,sizeof(struct floodPack));
        break;
      case 2:
        appFD= app3->fd;
        appID=app3->id;
        close(appFD);

        DELchannels(*portascelta);
        indexC--;
        Saldo=Saldo+(app3->stateP);
        printf("Canale %d con %c chiuso correttamente\n",appFD,appID);
        break;
    }
    pthread_mutex_unlock(&mutex_choice);
    //printf("UNLOCK CHANNELCONNECT\n\n" );
    return 0;

}

void* menu_exec(void *arg) {

  printf("CIAO PEER %c/%d, Premi: \n 1) Per collegarti ad un Peer\n 2) Per visualizzare i peer disponibili \n ",Pproto.name,Pproto.rec_port);
  if(indexC>0)
    printf("3) Per visualizzare gli state channels\n");

  scanf("%d", &key);
  switch (key) {
  case 1:
    fflush(stdin);
    pthread_create(&thread_action, NULL, peerConnect, NULL);
    pthread_join(thread_action, NULL);
    break;
  case 2:
    fflush(stdin);
    Pproto.flag = 1;
    break;
  case 3:
    fflush(stdin);
    printf("\tLista State Channels\n");
    printChannels();
    //BISOGNEREBBE SPOSTARE QUI LA CANCELLAZION E CON UNA SEMPLICE PRINTF, COME STA FATTO NELLA CHANNELSCONNECT
    pthread_mutex_unlock(&mutex_choice);
    printf("\n");
    break;
  default: printf("Scelta non riconosciuta\n" );break;
  }
  return 0;
}

int main(int argc, char **argv) {

  initChannel();
  indexC=0;
  char sendbuff[4096], recvbuff[4096];
  FD_ZERO(&fsetmaster);

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
  pthread_create(&thread_gestione,NULL,Gestione,NULL);

 system("clear");
  while (1) {

    pthread_mutex_lock(&mutex_choice);
    pthread_create(&thread_menu, NULL, menu_exec, NULL);
    pthread_join(thread_menu, NULL);
    sleep(2);
  }
  pthread_join(thread_receive, NULL);
  pthread_join(thread_peer, NULL);
  pthread_join(thread_gestione,NULL);
  exit(0);
}

/*
void sendMoney(){

  printf("A quale peer vuoi inviare denaro?(inserisci porta)\n");

  int x,fdX,sendALT=0; //x e' la porta, fdX e' il suo valore hash associat nell'array channels scanf("%d",&x); fdX=hashcode(x);

  if(FD_ISSET(fdX,&fset)){//CONTROLLO SE E' ANCORA ATTIVA LA CONNESSIONE
    write(fdX,1,sizeof(int)); //invio codice per identficare la richiesta d'invio denaro
    printf("Quanto vuoi inviare? Max:%d\n",channels[fdX].stateP );
    scanf("%d\n",&sendALT);
    if(sendALT>channels[fdX].stateP){
    }
    write(fdX,&sendALT,sizeof(int));
  }

}
*/