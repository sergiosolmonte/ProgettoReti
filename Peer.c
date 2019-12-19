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

struct Pacchetto{
    char ID;
    int  port;
    int  code;
};

int main(int argc, char **argv)
{
  int sockfd, n, nwrite, nread;
  char recvline[1025] ;
  struct sockaddr_in servaddr;
  char sendbuff[4096],recvbuff[4096];

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
  if (fgets(sendbuff, 4096, stdin) == NULL) {
	    return 1;                /* if no input just return */
  } else {                   /* else we have to write to socket */
	    nwrite = sendto(sockfd, sendbuff, strlen(sendbuff), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));
	    if (nwrite < 0) {      /* on error stop */
			printf("Errore in scrittura: %s", strerror(errno));
			return 1;
	    }
  }

  nread = recvfrom(sockfd, recvbuff, sizeof(recvbuff), 0, NULL, NULL);
  printf("Byte letti: %d\n", nread);
  if (nread < 0) {  /* error condition, stop client */
	  printf("Errore in lettura: %s\n", strerror(errno));
	  return 1;
  }
  recvbuff[nread] = 0;   /* else read is ok, write on stdout */
  if (fputs(recvbuff, stdout) == EOF) {
	  perror("Errore in scrittura su terminale");
	  return 1;
  }
  exit(0);
}
