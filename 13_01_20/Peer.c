#include <arpa/inet.h> /* IP addresses conversion utiliites */
#include <errno.h>     /* include error codes */
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h> /* include standard I/O library */
#include <stdlib.h>
#include <string.h> /* include erroro strings definitions */
#include <sys/select.h>
#include <sys/socket.h> /* socket library */
#include <sys/time.h>
#include <sys/types.h> /* predefined types */
#include <time.h>
#include <unistd.h> /* include unix standard library */

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
  int reached; // controllo se e' arrivato a destinazione
};

typedef struct floodPack FLOODPACK;

void *trackerConnect(void *);
void *channelConnect(void *);
void *menu_exec(void *);
void *Gestione(void *);
pthread_t thread_peer, thread_action, thread_receive, thread_menu, thread_set,
    thread_channel, thread_gestione;

pthread_mutex_t mutex_choice =
    PTHREAD_MUTEX_INITIALIZER; // UTILIZZATO IN TUTTE LE FUNZIONI APPARTENENTI
                               // AL MENU PER SINCORNIZZARE L'ESECUZIONE DELLO
                               // STESSO
pthread_mutex_t mutex_controllo =
    PTHREAD_MUTEX_INITIALIZER; // nel caso in cui mando flag=2 nella peer
                               // connect ho bisogno di un lock per evitare un
                               // controllo a vuoto
pthread_mutex_t mutex_fset =
    PTHREAD_MUTEX_INITIALIZER; // mutex che ci garantisce in mutua esclusione
                               // l'inserimento e la canzellazione nell'array
                               // fset
pthread_mutex_t mutex_flooding =
    PTHREAD_MUTEX_INITIALIZER; // MUTEX UTILIZZATO PER LA FUNZIONE DI  GESTIONE
                               // E PER SINCRONIZZARE INVIO E RICEZIONE DEI
                               // PACCHETTI floodPack
int fdApp;
int indexC;
int Saldo = 100;
int sockudp, n, nwrite, nread, i, socktcp, size_peer, listenfd, connfd, maxfd;
int IN_PAUSE, key;
char recvline[1025];
char recvline2[1025];
struct sockaddr_in servaddr, peer_list[10];
int control = 1, port;
struct ping_protocol Pproto;
struct ping_protocol *ArrayPeers;
fd_set fsetmaster;
fd_set appFset;
struct floodPack Fpack;

// void sendMoney(void *);
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

  if (choice == 3) { /*OH OH teletype*/
    printf("Quanto vuoi impegnare? (amount>=0)\n");
    scanf("%d", &amount1);

    pthread_mutex_lock(&mutex_fset);
    FD_SET(fd, &fsetmaster);

    if (maxfd < fd) {
      maxfd = fd;
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
    insertChannel(
        app2); // inserisco all'interno della mia lista di state channels
    // printChannels();
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

  // pthread_create(&thread_gestione,NULL,Gestione,NULL);

  while (1) {

    sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0,
           (struct sockaddr *)&servaddr, sizeof(servaddr));

    switch (Pproto.flag) {

    case 0:
      if (Pproto.lastPing == 0) {
        // Sono un nuovo peer
        printf("MANDO SEGNALE AL TRACKER\n");
        recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);
        printf("RICEVUTO clock aggiornato\n");
      } else {
        // PING SEMPLICE
        recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);
      }
      break;

    case 1:
      /* Richiesta lista peer disponibili*/
      free(ArrayPeers);
      recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL,
               NULL); // riceve prima il size dell'array
      ArrayPeers =
          realloc(ArrayPeers, size_peer * sizeof(struct ping_protocol));
      recvfrom(
          sockudp, ArrayPeers, size_peer * sizeof(struct ping_protocol), 0,
          NULL,
          NULL); // e poi i peer  direttamente dalla hash conenuta nel traker
                 // che essendo già un puntatore ad una struct
                 // non necessita di un indirizzamento
      printf("\tLista Peers Disponibili\n");

      for (i = 0; i < size_peer; i++) {

        printf("ID = %c Porta= %d\n", ArrayPeers[i].name,
               ArrayPeers[i].rec_port);
      }
      printf("\n");
      Pproto.flag = 0;
      //  printf("UNLOCK IN TRACKER CONNECT\n\n");
      pthread_mutex_unlock(&mutex_choice);

      break;

    case 2:
      free(ArrayPeers);
      recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL, NULL);
      ArrayPeers =
          realloc(ArrayPeers, size_peer * sizeof(struct ping_protocol));
      recvfrom(sockudp, ArrayPeers, size_peer * sizeof(struct ping_protocol), 0,
               NULL, NULL);
      Pproto.flag = 0;
      pthread_mutex_unlock(&mutex_controllo);
      break;
    }

    sleep(2);
  }
  // pthread_join(thread_gestione,NULL);
  return 0;
}

void *peerConnect(void *arg) {

  int porta;
  int amount = 0;
  int j, indice = 0;
  in_port_t porta_request;
  struct sockaddr_in toPeer;
  TRANSACTION *app4;
  int connfd, controllore = 0;
  pthread_mutex_lock(&mutex_controllo);

  printf("Inserisci la porta del peer al quale vuoi connetterti\n");
  scanf("%d", &porta);

  app4 = searchChannel(porta);

  Pproto.flag = 2;
  pthread_mutex_lock(&mutex_controllo);
  printf("DOPO LOCK\n");
  for (j = 0; j < size_peer; j++) {

    if (ArrayPeers[j].rec_port == porta) {
      controllore = 1;
      break;
    } // NELL'ULTIMA CONNESSIONE AL TRAKER ESISTE QUELLA PORTA 1 controllo
  }

  printf("PRIMA DELL'IF\n");
  if (controllore == 1) {
    // SE ESISTE QUESTO PEER
    if (app4 != NULL) { // 2 controllo
      // Se sono già connesso a questa porta in uno state channel
      pthread_create(&thread_channel, NULL, channelConnect, &porta);
      pthread_join(thread_channel, NULL);
      // printf("JOIN CHANNEL CONNECT\n");

    } else if (indexC != 0) { // HO DEGLI STATE CHANNEL APERTI E PROVERO A
                              // VEDERE SE LI POSSO USARE

      TRANSACTION *appInter = channels->pnext;

      pthread_mutex_lock(&mutex_flooding);
      printf("Quanto vuoi impegnare? (amount>0)\n");
      scanf("%d", &amount);

      Fpack.porta = porta;
      Fpack.n_hops = 0;
      Fpack.hops[0] = Pproto.rec_port;
      Fpack.saldoT = amount;
      Fpack.reached = 0;
      // SE QUESTO PEER PUO ESSERE RAGGIUNTO DA UN MIO STATE CHANNEL

      while (appInter != NULL) {

        if (appInter.stateP >= Fpack.saldoT)){
            write(appInter.fd, &Fpack, sizeof(struct floodPack));
            pthread_mutex_lock(&mutex_flooding);
          }

        if (Fpack.reached == 1) {

          int hop;
          printf("ALT INVIATI PASSANDO PER\n");
          for (hop = 0; hop < Fpack.n_hops; hop++) {
            printf(" %c ->", Fpack.hops[hop]);
          }
          printf("\n");

          break;

        } else { // SE REACHED==0

          appInter = appInter->pnext; // PASSO AL PROSSIMO STATE
        }
      }

    } else if (indexC == 0 ||
               Fpack.reached ==
                   0) { // CASO IN CUI NON HO ANCORA EFFETTUATO NESSUNA
                        // CONNESSIONE INDEXC=0 OPPURE SE LA RICERCA NON HA
                        // TROVATO UN CAMMINO REACHED=0

      if ((socktcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error");
        exit(1);
      }

      if (amount == 0) {
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
        FD_SET(socktcp, &fsetmaster);

        if (maxfd < socktcp)
          maxfd = socktcp;

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
          // printChannels();
          indexC++;

          // pthread_create(&thread_gestione,NULL,Gestione,&socktcp);
        }
      }
    }
    //  printf("UNLOCK IN PEER_CONNECT\n\n");
  } else {
    printf("PEER NON DISPONIBILE \n");
  }

  pthread_mutex_unlock(&mutex_controllo);
  pthread_mutex_unlock(&mutex_choice);
  return 0;
}

void *Gestione(void *arg) {

  printf("\n\nSONO IN GESTIONE\n\n");

  struct timeval timer;
  int o_select, indice;
  struct floodPack Fpackapp;

  while (1) {

    timer.tv_sec = 1;
    memcpy(&appFset, &fsetmaster, sizeof(fsetmaster));
    timer.tv_usec = 0;
    o_select = select(maxfd + 1, &appFset, NULL, NULL, &timer);

    //  printf("MAXFD %d\n",maxfd);
    if (o_select != 0) {
      for (indice = listenfd + 1; indice <= maxfd; indice++) {

        if (FD_ISSET(indice, &appFset)) {
          printf("DENTRO L'ISSET\n");
          read(indice, &, sizeof(struct floodPack));
          if (Fpackapp.port ==
              Pproto.rec_port) { // se sono io il destinatario O_DESTINATARIO

            printf("===== CONNESSIONE STATE CHANNEL =====");
            TRANSACTION *ptr;

            ptr = searchChannel(Fpackapp.[Fpackapp.n_hops]);
            ptr->stateP = (ptr->stateP) + Fpackapp.saldoT;

            Fpackapp.n_hops++;
            Fpackapp.hops[Fpackapp.n_hops] = Pproto.rec_port;
            Fpackapp.reached = 1;
            write(indice, &Fpackapp, sizeof(struct floodPack));
          } else if (Fpackapp.hops[0] == Pproto.rec_port &&
                     Fpackapp.reached ==
                         1) { // SE SONO IL MITTENTE E HO TROVATO IL PEER
                              // RICHIESTO O_PEERICHIESTOMITT

            TRANSACTION *ptr;
            ptr = searchChannel(Fpackapp.hops[1]);
            ptr->stateP = (ptr->stateP) - Fpackapp.saldoT;
            Fpack.reached = 1;
            pthread_mutex_unlock(&mutex_flooding);

          } else if (Fpackapp.hops[0] == Pproto.rec_port &&
                     Fpackapp.reached ==
                         0) { // SE SONO IL MITTENTE E NON HO TROVATO IL PEER
                              // O_PEERNONTROVMITT

            TRANSACTION *ptr;
            ptr = searchChannel(Fpackapp.hops[1]);
            ptr->stateP = (ptr->stateP) + Fpackapp.saldoT;
            Fpack.reached = 0;
            pthread_mutex_unlock(&mutex_flooding);

          } else { // SONO UN INTERMEDIARIO O_INTERMED

            printf("SONO UN INTERMEDIARIO, ciao %c \n", Pproto.name);
            TRANSACTION *ptr;
            int cisono = 0, p;
            for (p = 0; p == Fpackapp.n_hops; p++) {

              if (Fpackapp.hops[p] == Pproto.rec_port)
                cisono = 1;
              break;
            }
            if (cisono == 1 &&
                Fpackapp.reached ==
                    0) { // SE QUESTO PACK È GIA PASSATO DA ME, MA NON ABBIAMO
                         // TROVATO IL PEER RICHIESTO
              ptr = searchChannel(
                  Fpackapp
                      .hops[p - 1]); // Riaccredito il saldoT da me impegnato,
                                     // sul canale con il mio predecessore
              ptr->stateP = ptr->stateP + Fpackapp.saldoT;
              ptr = searchChannel(Fpackapp.hops[p + 1]);
              write(ptr->fd, &Fpackapp, sizeof(struct floodPack));

            } else {
              if (Fpackapp.n_hops < 5) {
                ptr = searchChannel(Fpackapp.dest_port);
                if (ptr != NULL) {
                  if (ptr->stateP >=
                      Fpackapp.saldoT) { // Se ho saldo disponibile sul canale
                                         // con il destinatario

                    ptr->stateP = ptr->stateP - Fpackapp.saldoT;
                    Fpackapp.n_hops++;
                    Fpackapp.hops[Fpackapp.n_hops] = Pproto.rec_port;
                    write(ptr->fd, &Fpackapp, sizeof(struct floodPack));

                    else { // SE NON HO SALDO SUFFICIENTE
                      ptr = searchChannel(
                          Fpackapp.hops[Fpackapp.n_hops]); // Ricerco il mio
                                                           // predecessore nei
                                                           // miei state channel
                      write(ptr->fd, &Fpackapp,
                            sizeof(struct floodPack)); // Invio il pacchetto con
                                                       // reached=0
                    }
                  } else { // Se io non ho un canale con il destinatario del
                           // pacchetto floodPack
                           // Devo fare flooding in avanti sui miei state
                           // channels
                    ptr = channels->pnext;
                    TRANSACTION *pointer;
                    while (ptr != NULL) {
                      if (!ricercaArray(
                              ptr->port,
                              Fpackapp)) { // se la porta non e' già presente
                                           // negli hop precedenti vado a
                                           // scrivere

                        ptr->stateP = ptr->stateP - Fpackapp.saldoT;
                        write(ptr->fd, &Fpackapp, sizeof(struct floodPack));
                        ptr = ptr->pnext;
                      } else { // se la porta e' gia presente negli hop
                               // precedenti non invio niente e vado avanti
                        ptr->pnext;
                      }
                    }
                  }
                } else { // UGUALE A 5
                  TRANSACTION *ptr;
                  ptr = searchChannel(Fpack.hops[4]);
                  write(ptr->fd, &Fpackapp, sizeof(FLOODPACK));
                }
              }
            }
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

    maxfd = listenfd;

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

  void *channelConnect(void *arg) {

    int keyC, appFD;
    int control;
    char appID;
    int appAmount;

    int *portascelta = (int *)arg;
    TRANSACTION *app3 = searchChannel(*portascelta);

    printf("%d\n", *portascelta);
    printf("VUOI:\n 1)INVIARE ALT \n 2)CHIUDERE IL CANALE \n");
    fflush(stdin);
    // printf("SALDO SU CANALE = %d ALT \n",app3->stateP );
    scanf("%d", &keyC);
    switch (keyC) {
    case 1:
      // Fpack.reached=1;
      printf("SALDO SU CANALE = %d ALT \n Quanto vuoi inviare? \n",
             app3->stateP); // CONTROLLO != NULL VEMIVA GIA FATTO NELLA PEER
                            // CONNECT
      fflush(stdin);
      scanf("%d", &appAmount);
      if (appAmount <= app3->stateP) {
        write(app3->fd, &Fpack, sizeof(struct floodPack));
      } else {
        printf("NON PUOI EFFETTUARE QUESTO MOVIMENTO, FONDI SUL CANALE "
               "INSUFFICIENTI\n");
      }
      break;
    case 2:
      appFD = app3->fd;
      appID = app3->id;
      close(appFD);
      DELchannels(*portascelta);
      indexC--;
      Saldo = Saldo + (app3->stateP);
      printf("Canale %d con %c chiuso correttamente\n", appFD, appID);
      break;
    }
    pthread_mutex_unlock(&mutex_choice);
    // printf("UNLOCK CHANNELCONNECT\n\n" );
    return 0;
  }

  void *menu_exec(void *arg) {

    printf("CIAO PEER %c/%d, Premi: \n 1) Per collegarti ad un Peer\n 2) Per "
           "visualizzare i peer disponibili \n ",
           Pproto.name, Pproto.rec_port);
    if (indexC > 0)
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
      // BISOGNEREBBE SPOSTARE QUI LA CANCELLAZION E CON UNA SEMPLICE PRINTF,
      // COME STA FATTO NELLA CHANNELSCONNECT
      pthread_mutex_unlock(&mutex_choice);
      printf("\n");
      break;
    default:
      printf("Scelta non riconosciuta\n");
      break;
    }
    return 0;
  }

  int main(int argc, char **argv) {

    initChannel();
    indexC = 0;
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
    pthread_create(&thread_gestione, NULL, Gestione, NULL);

    system("clear");
    while (1) {

      pthread_mutex_lock(&mutex_choice);
      pthread_create(&thread_menu, NULL, menu_exec, NULL);
      pthread_join(thread_menu, NULL);
      sleep(2);
    }
    pthread_join(thread_receive, NULL);
    pthread_join(thread_peer, NULL);
    pthread_join(thread_gestione, NULL);
    exit(0);
  }

  int searchArray(int port, struct floodPack a) {
    int j;
    for (j = 0; j < nHops; j++) {
      if (port == a[j])
        return 1;
    }
    return 0;
  }
  /*
  void sendMoney(){

    printf("A quale peer vuoi inviare denaro?(inserisci porta)\n");

    int x,fdX,sendALT=0; //x e' la porta, fdX e' il suo valore hash associat
  nell'array channels scanf("%d",&x); fdX=hashcode(x);

    if(FD_ISSET(fdX,&fset)){//CONTROLLO SE E' ANCORA ATTIVA LA CONNESSIONE
      write(fdX,1,sizeof(int)); //invio codice per identficare la richiesta
  d'invio denaro printf("Quanto vuoi inviare? Max:%d\n",channels[fdX].stateP );
      scanf("%d\n",&sendALT);
      if(sendALT>channels[fdX].stateP){
      }
      write(fdX,&sendALT,sizeof(int));
    }

  }
  */
