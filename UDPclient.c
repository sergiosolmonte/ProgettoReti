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

struct ping_protocol{

    char name;
    int rec_port;
    clock_t lastPing;
    int flag;


};
void * print_peer(void *);
pthread_t thread_peer;
pthread_mutex_t mutex_peer;
int sockfd, n, nwrite, nread,i,socktcp;
char recvline[1025] ;
char recvline2[1025] ;
struct sockaddr_in servaddr,peer_list[10],peer_connect;
int control=1;
struct ping_protocol Pproto;



void * print_peer(void * arg){

  while(1){

  printf("ENTRO NEL THREAD\n");
  if(Pproto.lastPing==0){
    //Sono un nuovo peer
    sendto(sockfd, &Pproto, sizeof(struct ping_protocol), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));
    printf("MANDO SEGNALE AL TRACKER\n");
    }


    pthread_mutex_lock(&mutex_peer);
    recvfrom(sockfd,&Pproto,sizeof(struct ping_protocol),0,NULL,NULL);
    printf("RICEVUTO struct ping\n");
    thread_mutex_unlock(&mutex_peer);
  /*
  sendto(sockfd, &control, sizeof(int), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));
  printf("Mandato ping \n");
  */
  if(Pproto.flag==1){
    recvfrom(sockfd,&size_peer,sizeof(int),0,NULL,NULL);  //riceve prima il size della lista
    recvfrom(sockfd,&peer_list,sizeof(peer_list),0,NULL,NULL); //e poi i peer
    printf("ricevo porte TRACKER\n");

    for(i=0;i<control;i++){printf("%d \n", ntohs(peer_list[i].sin_port));}
    Pproto.flag=0;
  }

    return 0;
  }



}

int main(int argc, char **argv)
{


  pthread_mutex_init (&mutex_peer, NULL);
  char sendbuff[4096],recvbuff[4096];
  fd_set fset;
  int fdInt[10000];


  if (argc != 2) {
    fprintf(stderr,"usage: %s <IPaddress>\n",argv[0]);
    exit (1);
  }
  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
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



  pthread_create(&thread_peer,NULL,print_peer,NULL);
  pthread_join(thread_peer,NULL);


  if ( (socktcp = socket(AF_INET, SOCK_STREAM,0)) < 0) {
    fprintf(stderr,"socket error");
    exit (1);
  }
  int choice;
  printf("vuoi collegare o ricevere?\n");
  scanf("%d",&choice);
  if(choice==1){
      in_port_t porta_request;
      int porta_int;
      printf("CON CHI TI VUOI COLLEGARE?");
      //display();
      scanf("%d",&porta_int);
      porta_request=htons(porta_int);
      peer_connect=servaddr;
      peer_connect.sin_port=porta_request;
      //servaddr.sin_port=porta_request;
      if (connect(socktcp,(struct sockaddr *) &peer_connect,sizeof(peer_connect))<0){
        perror("connect\n");

         snprintf(recvline,sizeof(recvline),"ciao sono il tuo peer");
         write(socktcp,recvline,strlen(recvline));
    }
  }else
  {
       int connfd;

    if ( bind(socktcp, (struct sockaddr *) &peer_connect, sizeof(servaddr)) < 0 ) {
        perror("bind");
        exit(1);
    }

      if(listen(socktcp,1024) < 0 ){
          perror("LISTEN");
      }

    if((connfd=accept(socktcp,(struct sockaddr *) NULL,NULL)<0)){
        perror("accept");
    }


      read(connfd,recvline2,strlen(recvline));
      printf("%s \n",recvline2);

  }



}
