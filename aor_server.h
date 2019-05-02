#ifndef AOR_SERVER_H_
#define AOR_SERVER_H_

#include  "pthread.h"


//---------------------------------------------------------------------------
//
//   File :      AOR_SERVER.H 
// 
//---------------------------------------------------------------------------

// Defines
#define 	_GNU_SOURCE
#define 	_FILEAOR		 		"regs.txt"
//#define 	_DEBUG

#define     _MAX_TIME_TRY_LOCK 		120
#define 	_CUR_NOFILE			    1000
#define 	_MAX_NOFILE			    1000
#define     _CUR_NPROC				1000
#define		_MAX_NPROC				1000

#define		_MAX_BUFFER			   	1500
#define 	_TCP_SRVPORT       	   	5000
#define 	_MAX_EVENTS 			10
#define 	_MAX_NUM_ENTRADAS 	   	40

#define 	_SLOTRANGE 			   	1000
#define     _THREAD_SLEEP			3000

#define 	_STAT_CLOSED     		0
#define 	_STAT_TALKING	  		1

#define 	_MAX_TOKENS				30
#define 	_MAX_AOR 				40
#define 	_MAX_JSON_OBJ 			1000
#define 	_MAX_AOR_FILE_NAME 		50
#define 	_MAX_CMD     			100
#define 	_MAX_VALUE   			500
#define 	_CHK_TIMEOUT 			1
#define 	_CONN_TIMEOUT 			10

typedef struct{
	char aor[_MAX_AOR+1];
	char json_obj[_MAX_JSON_OBJ+1];
	void *next;
} TREG;

typedef struct{
	int  flg_desligar;
	int   sock; 
	time_t dt_ult_msg;
	int   status; 
	int cont_erros;
} TLISTCONN;

// Function Prototypes
void bloqueiafilaentrada(int par);
void desbloqueiafilaentrada(int par);
int get_free_slot();
int tcpudpSend( int s, char* buffer, short tambuf );
void *ThReadSocket(void *arg);
int main  ( int argc, char **argv);
int SearchAORReg(char *aor, char *buffer, int *tamBuf);
int Load_AOR_Dumpfile();
void free_memory(int pos, TREG *reg);
void handlesigterm(__attribute__ ((unused)) int dummy);

// Global Variables
TREG *pListReg=NULL;
int  numReg=0; 
short gporta;
int listen_sock=0;
int epollfd=0;
pthread_mutex_t mutex_filaentrada;
int gListaEntradas[_MAX_NUM_ENTRADAS];
TLISTCONN gListaConn[_SLOTRANGE];


#endif  
