#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

struct transaction{
  int fd;
  char id;
  int port;
  int stateP;
  struct transaction *pnext;
  struct transaction *pprec;
};

typedef struct transaction TRANSACTION;
TRANSACTION *channels; // creo channels nella h che e' anche la testa della lista


TRANSACTION initChannel(){
  channels=(TRANSACTION*)malloc(1*sizeof(TRANSACTION));
  channels->pnext=NULL;
  channels->pprec=NULL;
}

void insertChannel(TRANSACTION ins){
  TRANSACTION *ptr=channels;
  while(ptr->p_next!=NULL){
    ptr=ptr->p_next;
  }
  TRANSACTION nuovo=(TRANSACTION*)malloc(1*sizeof(TRANSACTION));
  nuovo->fd=ins->d;
  nuovo->id=ins->id;
  nuovo->port=ins->port;
  nuovo->stateP=ins->stateP;
  ptr->p_next=nuovo;
  nuovo->pprec=ptr;
  nuovo->pnext=NULL;

}

void cancchannels(int portaDEL){ //portaDEL e' la porta del channels da eliminare
  TRANSACTION *ptr,*prec,*succ;
  ptr=channels;
  while(ptr->port!=portaDEL || ptr->pnext!=NULL){ //vado avanti fino a che non trovo la chiave o non mi trovo alla fine della lista
    ptr=ptr->pnext;
  }
  if(ptr->port!=portaDEL){ //controllo se sono arrivato alla fine della lista e non ho trovato la chiave
    printf("Porta non trovata\n", );
  }
  else{
    if(ptr->pprec==NULL){//se mi trovo in testa
      channels=ptr->pnext; //faccio scorrere la testa
      channels->pprec=NULL;
      free(ptr);
    }
    else if(ptr->pnext==NULL){//se mi trovo in coda
      prec=ptr->pprec;
      prec->pnext=NULL; //aggiorno l'ultimo nodo
      free(ptr);
    }
    else{ //cancellazione in mezzo
      prec=ptr->pprec;
      succ=ptr->pnext;
      prec->pnext=succ;//aggiorno i puntatori dei nodi adiacenti
      succ->pprec=prec;//facendo saltare quello eliminato
      free(ptr);
    }
    prec=ptr->pprec;

  }
}
