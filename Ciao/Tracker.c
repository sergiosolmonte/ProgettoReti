#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


#include "hash.h"

struct ping_protocol {
  char name;
  int rec_port;
  clock_t lastPing;
  int flag;
};

pthread_mutex_t mutex_peer;
void *insert_peers(void *);
void *pinging(void *);
struct ping_protocol Peer;
struct ping_protocol *ArrayPeers;
pthread_t thread_control, thread_ping;

int sock, n, control, sock;
int i = 0, k, j, fd = 0, peerPing;
struct sockaddr_in addr, in, peer[10];
char buff[4096];
int len = sizeof(in);
clock_t start;
clock_t end_t, total_t;
pthread_mutex_t mutex_ping = PTHREAD_MUTEX_INITIALIZER;
/*
  PROTOCOLLO:
  FLAG=0 ping
  FLAG=1
  FLAG=2
  FLAG=4
  lastPing=0 NEWPEER
*/
void *insert_peers(void *arg) {
  while (1) {
    // pthread_mutex_lock(&mutex_peer);

    // printf("Entrato nel While\n");

    recvfrom(sock, &Peer, sizeof(struct ping_protocol), 0,
             (struct sockaddr *)&in, &len);

    // printf("Richiesta Ricevuta\n");
    if (Peer.lastPing == 0) {
      printf("\n Received packet from %s:%d\n \n",inet_ntoa(in.sin_addr), ntohs(in.sin_port));
      /*
          BISOGNA INSERIRE GLI ELEMENTI NELL'HASH IN MODO TALE DA CAMBIARE IL
         PNG EFFETTIVO E NON SOLO QUELLO DA RESTITUIRE NEL ArrayPeers

      */
      start = clock();
      Peer.lastPing = start;
      pthread_mutex_lock(&mutex_ping);
      /*
      ArrayPeers[i] = Peer;
      */
      insert(Peer.rec_port);      //inserisce l'elemento nell'hash
      changeValue(Peer.rec_port,start);   //Nella funzione has.h serve ad aggiornare il ping

      i++;
      sendto(sock, &Peer, sizeof(struct ping_protocol), 0,(struct sockaddr *)&in, len);
      pthread_mutex_unlock(&mutex_ping);

      }
      else if (Peer.flag == 0)
    {
      int indexHash;
      pthread_mutex_lock(&mutex_ping);
      start = clock();
      Peer.lastPing = start;
      //indexHash=hashSearch(Peer.rec_port);
      changeValue(Peer.rec_port,start);
      //array[indexHash].
      //for (int j = 0; j < i;)

      pthread_create(&thread_ping, NULL, pinging, NULL);
      pthread_join(thread_ping, NULL);

      sendto(sock, &Peer, sizeof(struct ping_protocol), 0,(struct sockaddr *)&in, len);
      pthread_mutex_unlock(&mutex_ping);

    } else { // deve stampare
      pthread_mutex_lock(&mutex_ping);
      start = clock();
      //Peer.lastPing = start;
    //  ArrayPeers[hashSearch(Peer.rec_port)].lastPing=start;
      //i=getCapacity();
      changeValue(Peer.rec_port,start);
      free(ArrayPeers);
      ArrayPeers=(struct ping_protocol *)malloc(1 * sizeof(struct ping_protocol));
      printf("Tempo peer richiedente: %ld\n",  array[hashSearch(Peer.rec_port)].endPing);
      sendto(sock, &i, sizeof(int), 0, (struct sockaddr *)&in, len);
      int n;
      int k=0;
      for(n=0;n<getCapacity();n++){
          if(array[n].key>0){
            ArrayPeers[k].rec_port=array[n].key;ArrayPeers[k].lastPing=array[n].endPing;
            k++;
          }
      }
    //  display();
      sendto(sock, ArrayPeers, i*sizeof(struct ping_protocol), 0, (struct sockaddr *)&in, len);
      //free(ArrayPeers);
      pthread_mutex_unlock(&mutex_ping);
    }
  }
}

void *pinging(void *arg) {

    for (int n = 0; n < getCapacity(); n++) {

      if (array[n].key<1) {

        continue;

      } else {
          //start=clock();
        total_t = (double)(start - array[n].endPing) ;
        // Se il peer non è attivo da 10 secondi viene eliminato
        //printf("\nTempo start= %ld Tempo totale= %ld Tempo peer = %ld\n",start,total_t,ArrayPeers[n].lastPing);
        if (total_t > 2000) {
          remove_element(array[n].key);
          i--;
        }
      }
    }
  return 0;
}

int main(int argc, char **argv) {
ArrayPeers=(struct ping_protocol *)malloc(1 * sizeof(struct ping_protocol));
  init_array();
  start = clock();
  //i=getCapacity();
  // pthread_mutex_init (&mutex_peer, NULL);

  //  fd_set fset;
  //	int fdInt[10000];
  //  int max_fd;

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket");
    exit(1);
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(1024);

  pthread_create(&thread_control, NULL, insert_peers, NULL);
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }

  pthread_join(thread_control, NULL);
}
