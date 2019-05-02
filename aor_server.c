#ifdef  AOR_SERVER_ 
//---------------------------------------------------------------------------
//
//   Arquivo :      AOR_SERVER.c
//
//-------------------------------------------------------------------------------------------


// Includes
//------------------------------------
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h> 
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <poll.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/epoll.h>



// includes
#include "jsmn.h"
#include "aor_server.h"




int Load_AOR_Dumpfile()
{
	FILE *fd;
	char sline[_MAX_JSON_OBJ+1];
	char sfilename[_MAX_AOR_FILE_NAME];
	char scmd[_MAX_CMD+1];
	char svalue[_MAX_VALUE+1];
	int clines=0;
	jsmn_parser parser;
    jsmntok_t tokens[_MAX_TOKENS];
	int num_tokens=0;	
	TREG *preg=NULL;
	TREG *preg_ant=NULL;
	int x=0;
	
	sprintf(sfilename, ".//%s", _FILEAOR);
	
	fd = fopen(sfilename, "r");
	
	if(fd)
	{
		memset(sline,0x0,sizeof(sline));
		clines=0;
		while(fgets(sline,_MAX_JSON_OBJ, fd))
		{
			preg =  calloc(1,sizeof(TREG));
			if(preg)
			{
				// save the JSON Object
				strcpy(preg->json_obj, sline);
				
				// Take the AOR
				jsmn_init(&parser);
									
				             
				num_tokens = jsmn_parse(&parser, sline, strlen(sline), tokens, _MAX_TOKENS);
				if( num_tokens < 0 )
				{
#ifdef _DEBUG					
					printf("\n(Load_AOR_Dumpfile) ==> Parser error=%d\n",num_tokens);
#endif
					num_tokens=0;
				}
									
				x=1;
				while(x<num_tokens)
				{
					if( (tokens[x].type == JSMN_PRIMITIVE) || (tokens[x].type == JSMN_STRING) )
					{
						memset(scmd,0x0,sizeof(scmd));
						memset(svalue,0x0,sizeof(svalue));
						strncpy(scmd, sline+tokens[x].start, tokens[x].end - tokens[x].start);
						strncpy(svalue, sline+tokens[x+1].start, tokens[x+1].end - tokens[x+1].start);
#ifdef _DEBUG						
						printf("(Load_AOR_Dumpfile) JSON scmd [%s] svalue [%s]\n", scmd, svalue);
#endif
												
						if(!strcmp(scmd,"addressOfRecord"))
						{
							strcpy(preg->aor,svalue);
							break;
						}
					}
				
					x++;
				}
				
				if(!pListReg)
				{
					pListReg = preg;
				}
				else
				{
					preg_ant->next = (void *) preg;
				}
				
				clines++;
				preg_ant = preg;
				memset(sline,0x0,sizeof(sline));
			}
			else
			{			
				printf("\n(Load_AOR_Dumpfile) Error: no memory enough for dump file load !\n");
				break;
			}
		}
		
		
		fclose(fd);
	}
	
	numReg = clines;
	
#ifdef _DEBUG
	printf("(Load_AOR_Dumpfile) %d Lines read\n", clines);
#endif
	
	return clines;
}

int SearchAORReg(char *aor, char *buffer, int *tamBuf)
{
	int pos=0;
	int ret=0;
	TREG *preg=NULL;
	
	preg = pListReg;
	
	if( numReg && pListReg && aor && buffer )
	{
		while( preg && (pos <= numReg) )
		{
			if ( !strcmp(preg->aor, aor ) )
			{ // found !!				
				strcpy( buffer, preg->json_obj );
				*tamBuf = strlen(preg->json_obj);
				ret=1;
				break;
			}
			
			pos++;
			preg = (TREG *) preg->next;
		}
	}
	
	return ret;
}

void free_memory(int pos, TREG *reg)
{
	if ( (pos < numReg) && (reg) )
	{
		if(reg->next) free_memory(pos+1, reg->next);
#ifdef _DEBUG
	printf("(free_memory) Freeing memory pos=%d\n", pos);
#endif		
		free(reg);
	}
}


void handlesigterm(__attribute__ ((unused)) int dummy)
{
#ifdef _DEBUG
	printf("(handlesigterm) Finishing...\n");
#endif
	
   free_memory(0, pListReg);
   
   shutdown(listen_sock, SHUT_RDWR);
   close(listen_sock);
   
#ifdef _DEBUG
	printf("(handlesigterm) Finished !\n");
#endif   
   
   exit(0);
}

void IncluiFilaEntradas(int slot)
{
  int x=0;  
  
  for(x=0; x<_MAX_NUM_ENTRADAS; x++)
  {
	  if(gListaEntradas[x] < 0)
	  { // insert new slot
            gListaEntradas[x] = slot ;
#ifdef _DEBUG            
			printf("(IncluiFilaEntradas) inserted slot=%d on control list pos=%d\n", slot, x);
#endif
		    break;
	  }
		  
  }  
  
}

int RetiraFilaEntradas()
{
	int x=0,r=0;
	int slot=-1;
  
	for(x=0; x<_MAX_NUM_ENTRADAS; x++)
	{
        // Take off the first on list
		if ( gListaEntradas[x] >= 0) 	  
		{
			slot = gListaEntradas[x];
#ifdef _DEBUG			
			printf("(RetiraFilaEntradas) taked off slot=%d from control list pos=%d\n", slot, x);
#endif
			r=x+1;
			while(r<_MAX_NUM_ENTRADAS)
			{
				gListaEntradas[r-1] = gListaEntradas[r];
				r++;
			}
			
			if(r<_MAX_NUM_ENTRADAS)
				gListaEntradas[r]=-1;
		
			break;
		}
		else break;
		
	}

	return slot;
}

void bloqueiafilaentrada(int par)
{
	int erro=0;
	time_t dt_espera=0;

	erro = pthread_mutex_trylock(&mutex_filaentrada);
	if (erro) 
	{
		dt_espera=time(NULL);
		while(erro)
		{
			usleep(100);
			if( (time(NULL) - dt_espera) >  _MAX_TIME_TRY_LOCK )
			{
				printf("(bloqueiafilaentrada) Error: Excessive time of LOCK wait... Shutdown! [%d] [%ld] seconds\n", erro, (time(NULL) - dt_espera) );
				exit(-1);
			}
			erro = pthread_mutex_trylock(&mutex_filaentrada);
		}
		
	}

}

void desbloqueiafilaentrada(int par)
{
	int erro=0;
	
	erro = pthread_mutex_unlock(&mutex_filaentrada); 
	if (erro) printf("error on UNLOCK [%d]", erro); 
}


int main ( int argc, char *argv[] )
{
		struct rlimit  rl;
    	int  slot;
        struct  sockaddr_in addr;
		socklen_t addrlen;
		int x=0;
		int n=0;
		int found=0;
		int conn_sock=0;
		struct epoll_event ev, events[_MAX_EVENTS];
		pthread_t threadID=0;
		pthread_attr_t attr;
		int ret=0;
		int nfds=0;

        if(argc >= 2)
        {
            gporta = (short) atoi(argv[1]);
      	}
		else
            gporta = _TCP_SRVPORT;
		
        // Set limits and signals
        if(signal(SIGPIPE, SIG_IGN)==SIG_ERR)
           printf ("(main) Failed desativation SIGPIPE %d ", errno);
        
        signal(SIGTERM, handlesigterm);
        signal(SIGABRT, handlesigterm);
        signal(SIGINT, handlesigterm);

        rl.rlim_cur = _CUR_NOFILE;
        rl.rlim_max = _MAX_NOFILE;
        
		if (setrlimit(RLIMIT_NOFILE, &rl))
			printf ("(main) Failed setlimit NOFILE error=%d\n", errno);

        rl.rlim_cur = _CUR_NPROC;
        rl.rlim_max = _MAX_NPROC;
        
		if (setrlimit(RLIMIT_NPROC, &rl))
			printf ("(main) Failed setlimit NPROC error=%d\n", errno);
		
		
		errno=0;

		if (pthread_mutex_init(&mutex_filaentrada, NULL) != 0)
		{
			printf("\n (main) mutex_filaentrada init failed\n");
			exit(-1);
		}

		// Clean List
		for(x=0; x<_MAX_NUM_ENTRADAS; x++) 
			gListaEntradas[x]=-1;
		

		// Load Dump FILE
		if(Load_AOR_Dumpfile() <= 0)
		{
			printf("\n Error: none AOR loaded!! \n");
			exit(-2);
		}
		
		// Threads Activation
	    pthread_attr_init(&attr);
	    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		
		for(x=0; x<_MAX_EVENTS; x++)
		{ // Th. nr. iqual to events nr. from epoll
			ret= pthread_create(&threadID, &attr, ThReadSocket,(void *)x);
			if(ret)
			{
				printf("(ProcessaAgentesUsuarios) error %d on thread activation nr:%d\n", ret, x);
			}		
		}

        // Build TCP socket
		listen_sock = socket(PF_INET, SOCK_STREAM, 0);
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(gporta);
		addr.sin_addr.s_addr = INADDR_ANY;		
	
		addrlen = sizeof(addr);
	
		// epool
		while(1)
		{
			if ( bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == 0 )
			{
				ret = listen(listen_sock, 8000);
				if(!ret)
				{
					printf("(main) Listenning on port %d\n", gporta);
				}
				else
				{
					printf("(main) Error %d on listen... Closing !\n", errno);
					exit(-3);
				}
				
				
			
				epollfd = epoll_create1(0);
				if (epollfd == -1) {
					   printf("(main) epoll_create1 errno=%d\n", errno);
					   continue;
				}

				ev.events = EPOLLIN;
				ev.data.fd = listen_sock;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
					   printf("epoll_ctl: listen_sock errno=%d\n", errno);
					   continue;
				}

				for (;;) {
					errno=0;
					nfds = epoll_wait(epollfd, events, _MAX_EVENTS, _CHK_TIMEOUT);
					if (nfds == -1) {
					   printf("(main) epoll_wait errno=%d\n", errno);
					   sleep(1);
					   continue;
				    }
					else if(nfds == 0) {
						// verify connection timeout
						x=0;
					    while(x<_SLOTRANGE)
					    {
							if( (gListaConn[x].status == _STAT_TALKING) && ((time(NULL) - gListaConn[x].dt_ult_msg)>_CONN_TIMEOUT) )
							{ // timeout!								
								shutdown(gListaConn[x].sock, SHUT_RDWR);
								close(gListaConn[x].sock);
								gListaConn[x].status = _STAT_CLOSED;
#ifdef _DEBUG
								printf("(main) timeout socket=%d. Closed !\n", gListaConn[x].sock);
#endif
							}
							x++;
					    }
						
						continue;
					}
					
#ifdef _DEBUG
					printf("(main) nfds=%d errno=%d\n", nfds, errno);
#endif
					
				    for (n = 0; n < nfds; ++n) 
					{
#ifdef _DEBUG				    	
						printf("(main) n=%d events[n].data.fd=%d listen_sock=%d\n", n, events[n].data.fd, listen_sock);
#endif
						
						if (events[n].data.fd == listen_sock) 
						{
							// New Connection: accept, register and insert control list
							// accept
							errno=0;
							conn_sock = accept(listen_sock, (struct sockaddr *) &addr, &addrlen);
							if (conn_sock == -1) {
							   printf("(main) accept error errno=%d\n", errno);
							   continue;
							}
#ifdef _DEBUG							
							printf("(main) new connection conn_sock=%d\n", conn_sock);
#endif
							
							// insert epoll
							fcntl(conn_sock, F_SETFL, fcntl(conn_sock, F_GETFL, 0) | O_NONBLOCK);
							
							ev.events = EPOLLIN | EPOLLET;
							ev.data.fd = conn_sock;
							if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,  &ev) == -1) {
							   printf("(main) epoll_ctl: conn_sock error=%d \n", errno);
							   continue;
							}
							// find free slot in control list
							slot = get_free_slot();
							if(slot>=0)
							{
								memset(&(gListaConn[slot]),0x0,sizeof(TLISTCONN));
								gListaConn[slot].sock = conn_sock; // save the socket number
								gListaConn[slot].flg_desligar = 0;
								gListaConn[slot].status = _STAT_TALKING;
								gListaConn[slot].dt_ult_msg = time(NULL);
								gListaConn[slot].cont_erros=0;
								
#ifdef _DEBUG								
								printf("(main) new connection conn_sock=%d inserted in slot=%d\n", conn_sock, slot);
#endif
							}
							
							
							
						} else {
							// Connection already exist: only insert on queue for processing
							// Find the slot of this socket
							found=0;
							for(slot=0; slot<_SLOTRANGE; slot++)
							{
								if( gListaConn[slot].sock == events[n].data.fd) 
								{
									found=1;
									if(gListaConn[slot].status != _STAT_CLOSED)
									{
										bloqueiafilaentrada(1);
										IncluiFilaEntradas(slot);
										desbloqueiafilaentrada(1);
										break;
									}
									else
									{ //is closed: take off from epoll
#ifdef _DEBUG										
										printf("(main) events[n].data.fd=%d slot=%d CLOSED!\n", events[n].data.fd, slot);
#endif
										epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
										break;
									}
								}							
							}
							
							if(!found)
							{ // not found
#ifdef _DEBUG								
								printf("(main) events[n].data.fd=%d SLOT NOT FOUND listen_sock=%d\n", events[n].data.fd, listen_sock);
#endif

								// accept, register and insert control list
								// accept
								errno=0;
								conn_sock = accept(events[n].data.fd, (struct sockaddr *) &addr, &addrlen);
								if (conn_sock == -1) {
								   printf("(main) N accept errno=%d\n", errno);
								   continue;
								}
#ifdef _DEBUG								
								printf("(main) N new connection conn_sock=%d\n", conn_sock);
#endif
								
								// insert in epoll
								fcntl(conn_sock, F_SETFL, fcntl(conn_sock, F_GETFL, 0) | O_NONBLOCK);
								
								ev.events = EPOLLIN | EPOLLET;
								ev.data.fd = conn_sock;
								if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,  &ev) == -1) {
								   printf("(main) epoll_ctl: conn_sock errno=%d \n", errno);
								   continue;
								}
								// search free slot and insert in control list
								slot = get_free_slot();
								if(slot>=0)
								{
									memset(&(gListaConn[slot]),0x0,sizeof(TLISTCONN));
									gListaConn[slot].sock = conn_sock; // salva o socket 
									gListaConn[slot].flg_desligar = 0;
									gListaConn[slot].status = _STAT_TALKING;
									gListaConn[slot].dt_ult_msg = time(NULL);
#ifdef _DEBUG
									printf("(main)N new connection conn_sock=%d inserted no slot=%d\n", conn_sock, slot);
#endif
								}
								
							}
							
						}
					}
				}
			}
			else
			{
				printf("(main) Error %d on Binding to port %d\n", errno, gporta);
				sleep(5);
			}

		}
		   
		   
	return 0 ;
}

int get_free_slot()
{
   int x=0;
   int flg_conseguiu=0;

  
   x=0;
   while(x<_SLOTRANGE)
   {
	   if( !gListaConn[x].status )
	   {
		   gListaConn[x].status = _STAT_TALKING;
		   flg_conseguiu = 1;
		   break;
	   }
	   x++;
   }
   
   if(!flg_conseguiu) x=-1;
   
   return x;

}


// Thread: Process the TCP connection
void *ThReadSocket(void *arg)
{
    char buffer[_MAX_BUFFER];
    int bytes=0;
	int sent=0;
	int socket=0;
    int thread_id = (int)arg;
    char msgTCP[_MAX_BUFFER];
	int stat_dialogo = _STAT_CLOSED;
	int tamBuf=0;
	int slot=0;
	int x=0;
	
    if(signal(SIGPIPE, SIG_IGN)==SIG_ERR)
           printf ("(ThReadSocket) Thread[%d] Fail desativation SIGPIPE errno=%d ", thread_id, errno);	

    memset(buffer,0x0,sizeof(buffer));	
	
	
    while(1)
    {
        errno=0;
		socket=0;

		// Find SLOT with new FD from epoll available for reading
		bloqueiafilaentrada(2);
		slot = RetiraFilaEntradas();
		desbloqueiafilaentrada(2);
		
		if( (slot>=0) && (gListaConn[slot].sock > 0) )
		{ // new FD
			// take the data of this socket
			socket = gListaConn[slot].sock;

#ifdef _DEBUG			
			printf("(ThReadSocket) Thread[%d] processing slot=%d socket=%d\n", thread_id, slot, socket);
#endif
			
			
		    while(1)
			{  // process the new FD until the end 
				memset(msgTCP,0x0,sizeof(msgTCP));
				bytes = recv(socket, msgTCP, _MAX_BUFFER-1, MSG_DONTWAIT);
				
				if ( bytes == 0 )
				{ // Socket closed: set the control 
#ifdef _DEBUG					
					printf("(ReadAgentesUsuarios) Thread[%d] slot=%d empty (%d) socket=%d\n", thread_id, slot, bytes, socket);
#endif

					// take of from epoll
					epoll_ctl(epollfd, EPOLL_CTL_ADD, socket, NULL);
					
					// close the socket
					if( gListaConn[slot].status != _STAT_CLOSED )
					{
						stat_dialogo = _STAT_CLOSED;
						if(socket>0)
						{
							shutdown(socket, SHUT_RDWR);
							close(socket);
						}
					}
					
					 //free the slot
					gListaConn[slot].status = _STAT_CLOSED;
					
					break;
				}
				else if ( (bytes < 0) && (errno != EAGAIN) )
				{  // error on connection: close the socket and set the control
#ifdef _DEBUG
					printf("(ReadAgentesUsuarios) socket %d error %d. Closing \n",socket,errno);
#endif
					// take of from epoll
					epoll_ctl(epollfd, EPOLL_CTL_ADD, socket, NULL);
					
					// close the socket
					if( gListaConn[slot].status != _STAT_CLOSED )
					{
						stat_dialogo = _STAT_CLOSED;
						if(socket>0)
						{
							shutdown(socket, SHUT_RDWR);
							close(socket);
						}
					}
					
					 //free the slot
					gListaConn[slot].status = _STAT_CLOSED;
					
					break;
				}
				else if ( (bytes < 0) && (errno == EAGAIN) )
				{  // no more bytes
#ifdef _DEBUG					
					printf("(ThReadSocket) Thread[%d] read everything slot=%d socket=%d bytes=%d errno=%d\n", thread_id, slot, socket, bytes, errno);		
#endif				
					break; 
				}
				else if (bytes > 0)
				{ // data: process it
					gListaConn[slot].dt_ult_msg = time(NULL);
					msgTCP[bytes]=0x0;
					x=bytes-1;
					while(x >= 0)
					{
						if( (msgTCP[x]=='\n') || (msgTCP[x]=='\r') )
						{
							msgTCP[x]=0x0;
							bytes--;
							x--;
						}
						else break;
					}
#ifdef _DEBUG					
					printf("(ThReadSocket) Thread[%d] read bytes[%d] slot=%d socket=%d status=%d\n", thread_id, bytes, slot, socket, gListaConn[slot].status);
#endif
					
					memset(buffer,0x0,sizeof(buffer));
					tamBuf=0;
					if( SearchAORReg(msgTCP, buffer, &tamBuf) )
					{ // send JSON to client
						sent = tcpudpSend(gListaConn[slot].sock, buffer, tamBuf );
					}
					else
					{
						sprintf(buffer,"\r\n"); // send empty line
						sent = tcpudpSend(gListaConn[slot].sock, buffer, strlen(buffer));
					}
				}
			}
		}
		else usleep(_THREAD_SLEEP);
    }
	
	return 0;
}



int tcpudpSend(int s, char* buffer, short tambuf )
{
	int sent=0;
	int total=0;
	
#ifdef _DEBUG
	printf("(tcpudpSend) sending tambuf=%d socket=%d\n", tambuf, s);
#endif
	
	while(total < tambuf)
	{
		sent = send( s, (const void *) &(buffer[total]), tambuf-total, 0 );
		if (sent < 0)
		{
			printf("(tcpudpSend) error %d sending errno=%d\n", sent,errno);
			break;
		}
		total += sent;
	}
	
	
     return total ;
};

#endif 


