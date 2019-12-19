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
    time_t lastPing;
};

pthread_mutex_t mutex_peer;
void* insert_peers(void*);
void* pinging(void*);

int sock,n,control,sock;
int i=0,k,j,fd=0,peerPing;
struct sockaddr_in addr,in,peer[10];
char buff[4096];
int len=sizeof(in);


void *insert_peers(void * arg) {

while(1){ 
  //pthread_mutex_lock(&mutex_peer);
   recvfrom(sock, &control, sizeof(int), 0, (struct sockaddr *)&in, &len);
	printf("\n Received packet from %s:%d\n I=%d \n\n",inet_ntoa(in.sin_addr), ntohs(in.sin_port),i);
	peer[i]=in;
  insert(ntohs(in.sin_port));
	i++;
 //pthread_mutex_unlock(&mutex_peer);
  if(fd==i){
    printf("FD = %d i=%d",fd,i);
  }
  else{

  for(fd=0;fd<i;fd++){

  sendto(sock,&i,sizeof(int),0, (struct sockaddr *) &peer[fd],len);
  display();
	sendto(sock,&peer,sizeof(peer),0, (struct sockaddr *) &peer[fd],len);


  }


}
  //for(k=0;k<i;k++){printf("%d \n", ntohs(peer[k].sin_port));}

}
}

void* pinging(void* arg){

    while(1){

    }


}

int main(int argc, char **argv)
{

  init_array();
  pthread_t thread_control,thread_ping;
  pthread_mutex_init (&mutex_peer, NULL);

  fd_set fset;
	int fdInt[10000];
  int max_fd;

  if ( ( sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
    perror("socket");
    exit(1);
    }


  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(1024);

  if ( bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0 ) {
    perror("bind");
    exit(1);
  }


  pthread_create(&thread_control,NULL,insert_peers,NULL);
  pthread_create(&thread_ping,NULL,pinging,NULL);

  pthread_join(thread_control,NULL);
  pthread_join(thread_ping,NULL);


}
