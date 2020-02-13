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
#include <time.h> /*select quando crolla una connessione in c*/
#include <unistd.h> /* include unix standard library */

#include "lista.h"
#define SIGKILL 9
#define FALSE 0
#define TRUE 1

struct ping_protocol { //PROTOCOLLO COMUNICAZIONE TRACKER
  char name;
  int rec_port;
  clock_t lastPing;
  int flag;
};

struct Transaction { //STRUTTURA DI UNO STATE CHANNEL
  int fd;
  char id;
  int port;
  int stateP;
};

struct floodPack {  //PROTOCOLLO DI COMUNICAZIONE SU UNO STATE CHANNEL
  int dest_port;
  int n_hops;
  int hops[5];
  int saldoT;
  int reached; // controllo se e' arrivato a destinazione
};

typedef struct floodPack FLOODPACK;

int searchArray(int, FLOODPACK);
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

pthread_mutex_t mutex_tracker =PTHREAD_MUTEX_INITIALIZER; //MUTEX UTILIZZATO PER GESTIRE LA PRIMA CONNESSIONE AL TRAKER E L'ATTESA DELLA RISPOSTA



int fdApp;
int indexC;
int Saldo ;
int sockudp, n, i, socktcp, size_peer, listenfd, connfd, maxfd;
int IN_PAUSE, key;
char recvline[1025];
char recvline2[1025];
struct sockaddr_in servaddr, peer_list[10];
int control = 1, port;
int traspAmount; //viene utilizzata nel caso in cui retval=-1 , per evitare che nella peerconnect ci viene richiesto
struct ping_protocol Pproto;
struct ping_protocol *ArrayPeers;
fd_set fsetmaster;
fd_set appFset;
fd_set exceptfds;  //controllo delle eccezioni nele FD_ISSET
struct floodPack Fpack;
struct timeval tim;
struct timeval tv;
TRANSACTION * trackerOff;



// void sendMoney(void *);

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

  if (choice == 3) {
    printf("Quanto vuoi impegnare?\n AMOUNT = ");
    scanf("%d", &amount1);
    printf("\n" );
    pthread_mutex_lock(&mutex_fset);
    FD_SET(fd, &fsetmaster);

    if (maxfd < fd) {
      maxfd = fd;
    }
    pthread_mutex_unlock(&mutex_fset);

    if (amount1 <= Saldo)
      Saldo = Saldo - amount1;

    write(fd, &choice, sizeof(int));
    write(fd, &Pproto.name, sizeof(char));
    system("clear");


    TRANSACTION app2;
    app2.fd = fd;
    char id;
    read(fd, &id, sizeof(char)); // ricevo ID interlocutore
    app2.id = id;
    int port;
    read(fd, &port, sizeof(int)); // ricevo porta interlocutoreinsertChannel
    app2.port = port;
    app2.stateP = amount1;
    insertChannel(app2); // inserisco all'interno della mia lista di state channels
    indexC++;
    printf("\nMI SONO COLLEGATO CON %c ALLA SUA PORTA %d\n",app2.id, app2.port);

  } else if (choice == 4) {
    write(fd, &choice, sizeof(int));

    close(fd);
    system("clear");
    printf("Canale Chiuso\n");
  }
  sleep(2);
  pthread_mutex_unlock(&mutex_choice);
  IN_PAUSE = 0;

  return 0;
}

void *trackerConnect(void *arg) {

  pthread_mutex_lock(&mutex_tracker);
  tv.tv_sec=3;
  tv.tv_usec=0;
  if (setsockopt(sockudp, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Error");
  }




  int tentativi=0;
  int z;
  while (1) {
    sendto(sockudp, &Pproto, sizeof(struct ping_protocol), 0,(struct sockaddr *)&servaddr, sizeof(servaddr));





    switch (Pproto.flag) {

    case 0:
      if (Pproto.lastPing == 0) {
        // Sono un nuovo
          // Sono un nuovo peer

          z = recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);

          if( z < 0 ) {
            if(tentativi < 4) {
                  if( errno == EWOULDBLOCK || errno==EAGAIN) {
                    printf("Timeout - Tracker is off\n");
                    tentativi++;
                    break;
                }
                else
                  printf("errore in recvfrom");
            }
            else{
                if(indexC>0) //RIPRISTINA IL SALDO GENERALE CON QUELLO IMPEGNATO DURANTE LE TRANSAZIONI TRA PEER
                {  trackerOff=channels->pnext;
                  for(i=0;i<indexC;i++){
                    Saldo=Saldo+trackerOff->stateP;
                  }
                }
                printf("SALDO FINALE COMUNICAZIONE = %d \n\n", Saldo);
              exit(0);
            }
        }
        else{  //È ANDATA A BUON FINE LA REC, QUINDI IL PEER PUÒ RICEVERE L'HASHRES
          pthread_mutex_unlock(&mutex_tracker);
        }

      } else {
        // PING SEMPLICE

        z = recvfrom(sockudp, &Pproto, sizeof(struct ping_protocol), 0, NULL, NULL);
        if( z < 0 ) {
          if(tentativi < 4) {
                if( errno == EWOULDBLOCK || errno==EAGAIN) {
                  printf("Timeout - Tracker is off\n");
                  tentativi++;
                  break;
              }
              else
                printf("errore in recvfrom");
          }
          else{
            exit(0);
          }
      }

    }
      break; //break del case

    case 1:
      /* Richiesta lista peer disponibili*/

      free(ArrayPeers);
      recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL,
               NULL); // riceve prima il size dell'array
      ArrayPeers =malloc(size_peer * sizeof(struct ping_protocol));
      recvfrom(sockudp, ArrayPeers, size_peer * sizeof(struct ping_protocol), 0,NULL,NULL); // e poi i peer  direttamente dalla hash conenuta nel traker
                 // che essendo già un puntatore ad una struct
                 // non necessita di un indirizzamento
      printf("\tLista Peers Disponibili\n");

      for (i = 0; i < size_peer; i++) {

        printf("ID = %c Porta= %d\n", ArrayPeers[i].name,
               ArrayPeers[i].rec_port);
      }
      printf("\n");
      Pproto.flag = 0;
      pthread_mutex_unlock(&mutex_choice);

      break;

    case 2:
      free(ArrayPeers);
      recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL, NULL);
      ArrayPeers =  realloc(ArrayPeers, size_peer * sizeof(struct ping_protocol));
      recvfrom(sockudp, ArrayPeers, size_peer * sizeof(struct ping_protocol), 0, NULL, NULL);
      Pproto.flag = 0;
      pthread_mutex_unlock(&mutex_controllo);
      break;
    }

      sleep(2);
  }
  return 0;
}

void *peerConnect(void *arg) {

  int porta;
  int amount = 0;
  traspAmount=0;
  int j, indice = 0;
  in_port_t porta_request;
  struct sockaddr_in toPeer;
  TRANSACTION *app4;
  int connfd, control = 0;
  void *retValue;
  int retV;



  fflush(stdin);
  printf("\nInserisci la porta del peer al quale vuoi connetterti\n");
  printf("PORTA = ");
  scanf("%d", &porta);

  app4 = searchChannel(porta);


  Pproto.flag = 2;
  sleep(2);


  if(porta==Pproto.rec_port){
    control=0;
  }else{

    for (j = 0; j < size_peer; j++) {
      if (ArrayPeers[j].rec_port == porta) {
        control = 1;
        break;
      } // NELL'ULTIMA CONNESSIONE AL TRAKER ESISTE QUELLA PORTA 1 controllo
    }
  }

  if (control == 1) {
    // SE ESISTE QUESTO PEER
    if (app4 != NULL) { // 2 controllo
      // Se sono già connesso a questa porta in uno state channel
      pthread_create(&thread_channel, NULL, channelConnect, &porta);
      pthread_join(thread_channel, &retValue);
      retV=*(int*)retValue;  //per ottenere il valore di ritorno dalla thread_join nel tentativo di scambio diretto, cosi prima di creare un nuovo state
      // vede se ci può arrivare tramite i suoi collegamenti
      //printf("\nRETVALUE = %d\n",retV ); //retV = -1 se lo scambio su di un canale già attivo non avviene e =0 se riesce (tutto questo nella channelConnect)

    }


    if (indexC != 0 && retV!=1) { // HO DEGLI STATE CHANNEL APERTI E PROVERO A VEDERE SE LI POSSO USARE

      TRANSACTION *appInter = channels->pnext;
      amount=traspAmount;

      while(amount==0){
      printf("Quanto vuoi scambiare? (ALT>0)\n AMOUNT = ");
      fflush(stdin);
      scanf("%d", &amount);
      }

      Fpack.dest_port = porta;

      Fpack.hops[0] = Pproto.rec_port;
      Fpack.saldoT = amount;
      Fpack.reached = 0;

      if(amount>Saldo){

        printf("IMPOSSIBILE CREARE PER SALDO INSUFFICIENTE");
        pthread_mutex_unlock(&mutex_controllo);
        pthread_mutex_unlock(&mutex_choice);
        return 0;


      }
      // SE QUESTO PEER PUO ESSERE RAGGIUNTO DA UN MIO STATE CHANNEL

      //Controllo i miei state channels
      while (appInter != NULL) {

        Fpack.n_hops = 0;

        if (appInter->stateP >= Fpack.saldoT){
            write(appInter->fd, &Fpack, sizeof(struct floodPack));
            pthread_mutex_lock(&mutex_flooding);
        }
        else{
              printf("ALT INSUFFICIENTI SUL CANALE CON %c \n",appInter->id );
        }

        if (Fpack.reached == 1) {

          int hop;
          printf("ALT INVIATI PASSANDO PER\n");
          for (hop = 0; hop < Fpack.n_hops; hop++) {
            printf(" %d ->", Fpack.hops[hop]);
          }
          printf("\n");

          pthread_mutex_unlock(&mutex_controllo);
          pthread_mutex_unlock(&mutex_choice);
          return 0;

        } else { // SE REACHED==0
            printf("\nSCORRO LE ADIACENZE\n" );
          appInter = appInter->pnext; // PASSO AL PROSSIMO STATE
        }
      }

    }


      if (indexC == 0 || Fpack.reached == 0) {    // CASO IN CUI NON HO ANCORA EFFETTUATO NESSUNA
                                                  // CONNESSIONE INDEXC=0 OPPURE SE LA RICERCA NON HA
                                                  // TROVATO UN CAMMINO REACHED=0 E NON HO UNA CONNESSIONE DIRETTA ATTIVA
                                                  //CON QUEL PEER
      if(app4!=NULL){

          printf("\nDEVI PRIMA CHIUDERE IL CANALE N. %d APERTO CON %c SULLA PORTA %d PER POTER IMPEGNARE QUESTA SOMMA\n",app4->fd,app4->id, app4->port );
          pthread_mutex_unlock(&mutex_controllo);
          pthread_mutex_unlock(&mutex_choice);
          return 0;

      }

          if ((socktcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            fprintf(stderr, "socket error");
            exit(1);
          }

      if(Fpack.reached==0){
        printf("\nPERCORSO NON TROVATO, VERRÀ CREATO UNO STATE CHANNEL \n");

      }
      while(amount==0){
      if (amount == 0) {
        printf("\nQUANTI ALT VUOI IMPEGNARE? \n(amount>0)\n AMOUNT = ");
        fflush(stdin);
        scanf("%d", &amount);
      }
    }
      if(amount>Saldo){

        printf("\nIMPOSSIBILE CREARE PER SALDO INSUFFICIENTE\n");
        pthread_mutex_unlock(&mutex_controllo);
        pthread_mutex_unlock(&mutex_choice);
        return 0;


      }
      toPeer = servaddr;
      porta_request = htons(porta);
      toPeer.sin_port = porta_request;

      if (connect(socktcp, (struct sockaddr *)&toPeer, sizeof(toPeer)) < 0) {
        perror("connect\n");
      }

      // HO USATO ALT COME MONETA IN ONORE DEL MAGICO
      snprintf(recvline, sizeof(recvline),"\nCiao sono il peer %c e vorrei connettermi con %d ALT\n", Pproto.name, amount);
      int lung;
      lung = strlen(recvline);

      write(socktcp, &lung, sizeof(int));
      write(socktcp, recvline, strlen(recvline));

      // Ora verifichiamo l'accettazione
      int verify;
      char idPeerConn;
      read(socktcp, &verify, sizeof(int));
      if (verify == 4) {
        system("clear");
        printf("\n=====CONNESSIONE RIFIUTATA=====\n");
        close(socktcp);

      } else if (verify == 3) {
        system("clear");
        printf("\n=====CONNESSIONE ACCETTATA=====\n");
        printf("\n");

        pthread_mutex_lock(&mutex_fset);
        FD_SET(socktcp, &fsetmaster);

        if (maxfd < socktcp)
          maxfd = socktcp;

        pthread_mutex_unlock(&mutex_fset);

        if (amount <= Saldo)
          Saldo = Saldo - amount;

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
          indexC++;
        }
      }
    }
  } else {
    printf("\nPEER NON DISPONIBILE \n");
  }

  pthread_mutex_unlock(&mutex_controllo);
  pthread_mutex_unlock(&mutex_choice);
  return 0;
}

void *Gestione(void *arg) {

  struct floodPack Fpackapp;
  struct timeval timer;
  int o_select, indice,cisono,p;
  TRANSACTION* ptr,*punt;

  while (1) {

    timer.tv_sec = 1;
    memcpy(&appFset, &fsetmaster, sizeof(fsetmaster));
    memcpy(&exceptfds,&fsetmaster,sizeof(fsetmaster));
    timer.tv_usec = 0;
    o_select = select(maxfd + 1, &appFset, NULL, &exceptfds, &timer);


    if (o_select >= 0) {
    //Controlliamo tutti i descrittori nell'fset
      for (indice = listenfd + 1; indice <= maxfd; indice++) {

        if (FD_ISSET(indice, &appFset)) {
          pthread_cancel(thread_menu);
          //printf("DENTRO L'ISSET\n");
          memset(&Fpackapp,0,sizeof(struct floodPack));
          read(indice, &Fpackapp, sizeof(struct floodPack));


          if(Fpackapp.dest_port==0){
            printf("IL PEER NON COMUNICA PIÙ SULLA PORTA %d\n",getPort(indice) );                            // QUESTO CONTROLLO CI SERVE A CAPIRE SE UN PEER È MORTO E QUINDI SUL SUO DESCRITTORE RESTA BLOCCATO UN pacchetto
            FD_CLR(indice,&fsetmaster);   //  INIZIALIZZATO A 0;
            indexC--;
            int portApp=getPort(indice);  //CERCO LA PORTA DA ELIMINARE NELLA LISTA ASSOCIATA A INDICE
            ptr= searchChannel(portApp);
            Saldo=Saldo+ptr->stateP;
            DELchannels(portApp);
            pthread_mutex_unlock(&mutex_choice);
          }
          else{

          // se sono io (Pproto) il destinatario
          if (Fpackapp.dest_port == Pproto.rec_port) {
            system("clear");
            printf("===== CONNESSIONE STATE CHANNEL =====\n");


            ptr = searchChannel(Fpackapp.hops[Fpackapp.n_hops]);
            //Accredito sullo state channel dal quale mi è arrivato il pacchetto FLOODPACK, il saldo che mi è stato inviato
            ptr->stateP = (ptr->stateP) + Fpackapp.saldoT;
            //Aggiungo me stesso al pacchetto e incremento il numero di hops
            Fpackapp.n_hops++;
            Fpackapp.hops[Fpackapp.n_hops] = Pproto.rec_port;
            Fpackapp.reached = 1;
            //Invio la risposta al descrittore dal quale ho ricevuto il pacchetto
            write(indice, &Fpackapp, sizeof(struct floodPack));
            pthread_mutex_unlock(&mutex_choice);
            //pthread_mutex_unlock(&mutex_controllo);
          }
          //Se il pacchetto è già arrivato al destinatario
          else if(Fpackapp.reached==1){

              if (Fpackapp.hops[0] == Pproto.rec_port){   //SE SONO IO QUELLO CHE HA RICHIESTO LA CONNESSIONE, DEVO SBLOCCARE IL MUTEX BLOCCATO NELLA PEER CONNECT

                  printf("MI È ARRIVATA LA RISPOSTA DA %d, HA RICEVUTO %d ALT\n",Fpackapp.dest_port, Fpackapp.saldoT);
                  ptr = searchChannel(Fpackapp.hops[1]);
                  ptr->stateP = (ptr->stateP) - Fpackapp.saldoT;
                  Fpack=Fpackapp;
                  Fpack.reached = 1;
                  pthread_mutex_unlock(&mutex_flooding);

              }
              //SONO UN INTERMEDIARIO E DEVO MANDARE IL PACCHETTO INDIETRO
              else{
                  printf("SONO UN INTERMEDIARIO, ciao %c porta richiesta: %d \n", Pproto.name,Fpackapp.dest_port);



                  for (p = 0; p <= Fpackapp.n_hops; p++) {
                      if (Fpackapp.hops[p] == Pproto.rec_port){
                        break;
                      }
                  }

                  ptr = searchChannel(Fpackapp.hops[p+1]);
                  ptr->stateP=ptr->stateP-Fpackapp.saldoT;
                  ptr = searchChannel(Fpackapp.hops[p-1]);
                  ptr->stateP=ptr->stateP+Fpackapp.saldoT;
                  printf("IL MIO PREDECESSORE È %c\n",ptr->id );

                  write(ptr->fd,&Fpackapp,sizeof(struct floodPack));
                  pthread_mutex_unlock(&mutex_choice);
                  //pthread_mutex_unlock(&mutex_controllo);
               }

        }
        //REACHED==0
        else{
               if (Fpackapp.hops[0] == Pproto.rec_port){ // SONO IL MITTENTE MA NON HO TROVATO IL PEER/STABILITO LA CONNESSIONE
                     printf("SONO IL MITTENTE E NON HO TROVATO PASSANDO PER %d\n", Fpackapp.hops[1]);                           // O_PEERNONTROVMITT
                     Fpack=Fpackapp;
                     Fpack.reached = 0;
                     pthread_mutex_unlock(&mutex_flooding);
               }
               else{ //SE NON SONO IL MITTENTE ALLORA SONO UN INTERMEDIARIO

                     printf("SONO UN INTERMEDIARIO,REACHED 0 ciao %c porta richiesta: %d \n", Pproto.name,Fpackapp.dest_port);


                     cisono=0;

                     for (p = 0; p <= Fpackapp.n_hops; p++) {
                         if (Fpackapp.hops[p] == Pproto.rec_port){
                           cisono = 1;
                           break;
                          }
                      }


                     if(cisono==1){

                         ptr = searchChannel(Fpackapp.hops[p-1]);
                         printf("SONO INTERMEDIARIO E STO TORNANDO INDIETRO\n" );
                         write(ptr->fd,&Fpackapp,sizeof(struct floodPack));
                     }
                      else{ //NON CI SONO NEGLI HOPS IL PACCHETTO DEVE AVANZARE
                         if(Fpackapp.n_hops < 3)  {
                                  printf("\n\n NON CI SONO, n_hops= %d\n\n",Fpackapp.n_hops);
                                Fpackapp.n_hops++;
                                Fpackapp.hops[Fpackapp.n_hops]=Pproto.rec_port;
                                punt=searchChannel(Fpackapp.hops[Fpackapp.n_hops-1]);
                                ptr=searchChannel(Fpackapp.dest_port);

                                //Se ho uno state channel con il destinatario
                                if (ptr != NULL) {
                                      if (ptr->stateP >= Fpackapp.saldoT) { // Se ho saldo disponibile sul canale
                                                                            // con il destinatario, faccio flooding in avanti
                                        printf("HO SALDO E COMUNICO CON %c \n",ptr->id);
                                        //ptr->stateP = ptr->stateP - Fpackapp.saldoT;
                                        //punt->stateP=punt->stateP+Fpackapp.saldoT;
                                        write(ptr->fd, &Fpackapp, sizeof(struct floodPack));

                                        }
                                        else { // SE NON HO SALDO SUFFICIENTE
                                          printf("SENZA ALT SULLO STATE CHANNEL\n" );
                                          //ptr = searchChannel(Fpackapp.hops[Fpackapp.n_hops-1]);
                                          write(punt->fd, &Fpackapp,sizeof(struct floodPack));
                                                                                              /*Ricerco il mio predecessore nei
                                                                                               miei state channel Invio il pacchetto con reached=0*/
                                        }
                                  }
                                  else{ //NON HO UN COLLEGAMENTO DIRETTO CON IL PEER  DESTINATARIO RICHEISTO
                                      ptr=channels->pnext;

                                      while(ptr!=NULL){


                                                if(ptr==channels->pnext && ptr->pnext==NULL){
                                                    write(ptr->fd,&Fpackapp,sizeof(struct floodPack));
                                                }

                                                 if(ptr!=punt){

                                                   write(ptr->fd,&Fpackapp,sizeof(struct floodPack));
                                                 }

                                                 ptr=ptr->pnext;


                                       }
                                  }

                                }
                                else{ //SUPERATO LIMITE DI HOPS >3


                                      Fpackapp.n_hops++;
                                      Fpackapp.hops[Fpackapp.n_hops]=Pproto.rec_port;
                                      ptr=searchChannel(Fpackapp.hops[3]);
                                      write(ptr->fd,&Fpackapp,sizeof(struct floodPack));
                                }
                     }
                     pthread_mutex_unlock(&mutex_choice);

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

      printf("\n=====TENTATIVO DI CONNESSIONE=====\n\n");

      fdApp = connectfd;

      pthread_create(&thread_set, NULL, peerAccept, NULL);
      pthread_join(thread_set, NULL);
    }
    sleep(2);
    system("clear");
    exit(0);
  }

void *channelConnect(void *arg) {

    int keyC, appFD;
    int control;
    char appID;
    int appAmount;
    static int retValue=0;

    int *portascelta = (int *)arg;
    TRANSACTION *app3 = searchChannel(*portascelta);

    printf("VUOI:\n 1) INVIARE ALT \n 2) CHIUDERE IL CANALE \n 3) TORNA AL MENÙ \n");
    fflush(stdin);
    scanf("%d", &keyC);
    switch (keyC) {
    case 1:

      printf("SALDO SU CANALE = %d ALT \n Quanto vuoi inviare? \n",
             app3->stateP); // CONTROLLO != NULL VEMIVA GIA FATTO NELLA PEER
                            // CONNECT

      fflush(stdin);
      scanf("%d", &appAmount);
      if (appAmount <= app3->stateP) {
        Fpack.dest_port=*portascelta;
        Fpack.n_hops=0;
        Fpack.reached=0;
        Fpack.saldoT=appAmount;
        Fpack.hops[0]=Pproto.rec_port;
        write(app3->fd, &Fpack, sizeof(struct floodPack));
        retValue=0;
        system("clear");
      } else {
        system("clear");
        printf("NON PUOI EFFETTUARE QUESTO MOVIMENTO, FONDI SUL CANALE INSUFFICIENTI\n");
        traspAmount=appAmount;
        retValue=-1;
      }

      break;
    case 2:
      appFD = app3->fd;
      appID = app3->id;
      close(appFD);
      Saldo = Saldo + (app3->stateP);
      printf("Il tuo saldo totale: %d\n", Saldo);
      DELchannels(*portascelta);
      FD_CLR(appFD,&fsetmaster);
      indexC--;

      system("clear");
      printf("Canale %d con %c chiuso correttamente\n", appFD, appID);

      pthread_mutex_unlock(&mutex_controllo);

      //printf("SALDO TOTALE = %d\n", Saldo );
      break;

      case 3:
                pthread_mutex_unlock(&mutex_controllo);
                break;
    }
    //printf("ReTval dopo case %d\n",retValue);
    if(retValue==-1){
      //pthread_cancel(thread_action);
      //pthread_mutex_unlock(&mutex_choice);
      return (void*)&retValue;
      //return (void*)retValue;
    }else{

      pthread_cancel(thread_action);
      pthread_mutex_unlock(&mutex_choice);
      //return (void*)retValue;
      return (void*)&retValue;

    }


  }

void *menu_exec(void *arg) {

    printf("CIAO PEER %c/%d, Premi: \n 1) Per collegarti ad un Peer\n 2) Per "
           "visualizzare i peer disponibili \n ",
           Pproto.name, Pproto.rec_port);

    if (indexC > 0)
      printf("3) Per visualizzare gli state channels\n");


    int keyC,portToRem,appFD;
    char appID;
    TRANSACTION *app3;
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
      if(indexC<=0){
        printf("Scelta non riconosciuta\n");
        sleep(1);
        system("clear");
        pthread_mutex_unlock(&mutex_choice);
        break;
      }
      fflush(stdin);
      printf("\tLista State Channels\n");
      printChannels();
      printf("VUOI TERMINARE QUALCHE CANALE? \n 1) SI \n 2) NO\n");
      scanf("%d",&keyC);

      if(keyC==1){
            fflush(stdin);
            printf("\n INSERIRE LA PORTA DEL CANALE DA CHIUDERE \n");
            scanf("%d",&portToRem);
            app3 = searchChannel(portToRem);
            appFD = app3->fd;
            appID = app3->id;
            close(appFD);
            Saldo = Saldo + (app3->stateP);
            printf("Il tuo saldo totale: %d\n", Saldo);
            DELchannels(portToRem);
            FD_CLR(appFD,&fsetmaster);
            indexC--;
            system("clear");
            printf("Canale %d con %c chiuso correttamente\n", appFD, appID);
      }
      fflush(stdin);
      pthread_mutex_unlock(&mutex_choice);
      printf("\n");
      break;
    default:
      printf("Scelta non riconosciuta\n");
      sleep(1);
      system("clear");
      pthread_mutex_unlock(&mutex_choice);
      break;
    }
    return 0;
  }

int main(int argc, char **argv) {

    initChannel();
    indexC = 0;
    char sendbuff[4096], recvbuff[4096];
    char name;
    FD_ZERO(&fsetmaster);
    Saldo=100;
    int hashRes=0;
    int schif=0;


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
    scanf("%c", &name);
  while(hashRes==0){

    Pproto.name=name;
    printf("QUALE PORTA USI PER RICEVERE?\n");
    scanf("%d", &Pproto.rec_port);
    Pproto.lastPing = 0;
    Pproto.flag = 0;


    pthread_create(&thread_peer, NULL, trackerConnect, NULL);
    sleep(1);
    pthread_mutex_lock(&mutex_tracker);
    recvfrom(sockudp,&hashRes,sizeof(int),0,NULL,NULL);
    printf("ASPETTO DISPONIBILITÀ TRACKER\n" );



    if(hashRes==1){

    pthread_create(&thread_receive, NULL, openPort, NULL);
    pthread_create(&thread_gestione, NULL, Gestione, NULL);
    pthread_mutex_lock(&mutex_flooding);
    sleep(1); //per ottimizzazione grafica
    system("clear");
    while (1) {

      pthread_mutex_lock(&mutex_choice);
      printf("SALDO= %d\n",Saldo );
      pthread_create(&thread_menu, NULL, menu_exec, NULL);
      pthread_join(thread_menu, NULL);
      sleep(2);
    }
    pthread_join(thread_receive, NULL);
    pthread_join(thread_peer, NULL);
    pthread_join(thread_gestione, NULL);

    }
    else{
        if(schif>1)
        {printf("\n\tCHE PROBLEMI HAI?\n");}

        printf("\nPORTA GIÀ UTILIZZATA\n");
        pthread_cancel(thread_peer);
        pthread_mutex_unlock(&mutex_tracker);
        pthread_join(thread_peer,NULL);
        schif++;
    }
}
    exit(0);

  }

int searchArray(int port, struct floodPack a) {
    int j;
    for (j = 0; j < a.n_hops; j++) {
      if (port == a.hops[j])
        return 1;
    }
    return 0;
  }
