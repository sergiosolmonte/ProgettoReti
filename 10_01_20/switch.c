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
                                free(ArrayPeers);
                                break;

                    case 2:
                                recvfrom(sockudp, &size_peer, sizeof(int), 0, NULL, NULL);
                                ArrayPeers=realloc(ArrayPeers,size_peer * sizeof(struct ping_protocol));
                                recvfrom(sockudp, ArrayPeers, size_peer*sizeof(struct ping_protocol), 0, NULL,NULL);
                                Pproto.flag = 0;
                                break;

         }



        sleep(2);
}
