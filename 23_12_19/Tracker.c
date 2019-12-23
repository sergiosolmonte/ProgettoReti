#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>

#include "hash.h"

struct ping_protocol{
    char name;
    int rec_port;
    clock_t lastPing;
    int flag;
};

pthread_mutex_t mutex_peer;
void* insert_peers(void*);
void* pinging(void*);
struct ping_protocol Peer,ArrayPeers[10];
pthread_t thread_control,thread_ping;

int sock,n,control,sock;
int i=0,k,j,fd=0,peerPing;
struct sockaddr_in addr,in,peer[10];
char buff[4096];
int len=sizeof(in);


void *insert_peers(void * arg) {
  while(1){
    //pthread_mutex_lock(&mutex_peer);

  //printf("Entrato nel While\n");

     recvfrom(sock, &Peer, sizeof(struct ping_protocol), 0, (struct sockaddr *)&in, &len);

    // printf("Richiesta Ricevuta\n");
     if(Peer.lastPing==0){
       printf("\n Received packet from %s:%d\n I=%d \n\n",inet_ntoa(in.sin_addr), ntohs(in.sin_port),i);
       Peer.lastPing=clock();
       ArrayPeers[i]=Peer;
       insert(Peer.rec_port);
       i++;
       sendto(sock, &Peer, sizeof(struct ping_protocol), 0, (struct sockaddr *) &in, len);
     }
     else if(Peer.flag==0){
        Peer.lastPing=clock();
        sendto(sock, &Peer, sizeof(struct ping_protocol), 0, (struct sockaddr *) &in, len);

          }else{

                Peer.lastPing=clock();
                sendto(sock,&i,sizeof(int),0, (struct sockaddr *)&in,len);
                display();
                sendto(sock,&ArrayPeers,sizeof(ArrayPeers),0, (struct sockaddr *) &in,len);

              }
    }
}

void* pinging(void* arg){

    while(1){}
}

int main(int argc, char **argv)
{

  init_array();

  //pthread_mutex_init (&mutex_peer, NULL);

  fd_set fset;
	int fdInt[10000];
  int max_fd;

  if ( ( sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
    perror("socket");
    exit(1);
    }
//altobelli focess

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(1024);

    pthread_create(&thread_control,NULL,insert_peers,NULL);
  if ( bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0 ) {
    perror("bind");
    exit(1);
  }


  pthread_join(thread_control,NULL);

}
