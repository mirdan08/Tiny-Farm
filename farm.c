#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define HOST "127.0.0.1"
#define PORT 1033
#define DE 0  //Data Exchange
#define CC -1 //Close Connection
#define MAX_SIZE 255 //lunghezza massima nome di un file
#define STOP "<<END>>"
typedef struct {
	bool* end_server;
	int* pind;
	int* cind;
	pthread_mutex_t * pcmux;
	char ** pcbuff;
	sem_t * free_slots;
	sem_t * data_items;
	int * qlen;
} th_args;
//thread gestore del segnale SIGINT
void* handler(void*args){
	//blocco tutti i segnali a parte SIGINT
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask,SIGINT);
	pthread_sigmask(SIG_BLOCK,&mask,NULL);
	sigemptyset(&mask);
	sigaddset(&mask,SIGINT);
	int s;
	sigwait(&mask,&s);
	bool* interrupt = (bool*) args;
	*interrupt =true;
	return NULL;
}
//la funzione invia dati nel server nel seguente modo
//lunghezza in byte del messaggio (4 byte) 
//somma calcolata (8 byte) 
//nome file ( lunghezza messaggio - 8 byte della somma)
//se la lunghezza e' negativa significa che voglio terminare la connessione 
//ed invia solo la lunghezza
void send_data(const int* socketfd,int length,long sum,char* name){
	int tmp;
	tmp=htonl(length);
	writen(*socketfd,&tmp,sizeof(int));
	if (length < 0) return ;
	tmp = htonl((int)(sum>>32) );
	writen(*socketfd,&tmp,sizeof(int));
	tmp = htonl((int) sum);
	writen(*socketfd,&tmp,sizeof(int));
	writen(*socketfd,name,(length-sizeof(long) )*sizeof(char));
}

int open_connection(const char* host,const uint16_t port){
	struct sockaddr_in addr;
	int socketfd=socket(AF_INET,SOCK_STREAM,0);
	if(socketfd<0)
		return -1;
	addr.sin_family= AF_INET;
	addr.sin_port= htons(port);
	addr.sin_addr.s_addr= inet_addr(host);
	if( connect(socketfd,(struct sockaddr*)&addr,sizeof(addr))<0 )
		return -1;
	return socketfd;
}

//corpo del thread worker
void* wbody(void* args){
	//gestione segnali
	sigset_t mask;
	sigfillset(&mask);
	pthread_sigmask(SIG_BLOCK,&mask,NULL);
	//apertura connessione
	int socketfd=open_connection(HOST,PORT);
	if (socketfd<0){return NULL; }
	
	int tmp;
	th_args* d=(th_args*)args;
	char* name=malloc(MAX_SIZE*sizeof(char));
	//invio messaggio per inizio scambio dati
	tmp=htonl(DE);
	if(writen(socketfd,&tmp,sizeof(int)) < 0) {
		close(socketfd);
		pthread_exit(0);
	}
	while(true){
		//attesa nome file dal buffer prod/cons
		xsem_wait(d->data_items,QUI);
		xpthread_mutex_lock(d->pcmux,QUI);
		strcpy(name,d->pcbuff[ *d->cind % *d->qlen ]);
		*d->cind += 1;
		xpthread_mutex_unlock(d->pcmux,QUI);
		xsem_post(d->free_slots,QUI);
		//controllo messaggio di terminazione del thread 
		//se si segnala anche al thread del server di terminare
		if(strcmp(STOP,name)==0){
			send_data(&socketfd,CC,0,STOP);
			break;
		}
		//apertura,lettura e calcolo della somma 
		FILE* f=fopen(name,"rb");
		if(f!=NULL){
			long sum=0;
			long n=0;
			int i=0;
			while( fread(&n,sizeof(long),1,f) == 1 )
				sum += i++* n;
			//invio dei dati al server
			fclose(f);
			send_data(&socketfd,strlen(name)+1+sizeof(long),sum,name);
		}
	}
	close(socketfd);
	free(name);
	return NULL;
}
bool check_server(){
	int socketfd=open_connection(HOST,PORT);
	if(socketfd<0) return false;
	int tmp=htonl(DE);
	writen(socketfd,&tmp,sizeof(int));
	tmp=htonl(CC);
	writen(socketfd,&tmp,sizeof(int));
	close(socketfd);
	return true;
}
void close_server(){
	int socketfd= open_connection(HOST,PORT);
	//invio messaggio di terminazione
	int tmp=htonl(CC);
	writen(socketfd,&tmp,sizeof(int));
	close(socketfd);
}

int main(int argc, char *argv[])
{

	//imposto gestione dei segnali
	sigset_t mask;
	sigfillset(&mask);
	//sigaddset(&mask,SIGINT);
	pthread_sigmask(SIG_BLOCK,&mask,NULL);
  // controlla numero argomenti
  if(argc<2) {
      printf("Uso: %s file [file ...] \n",argv[0]);
      return 1;
  }
  //gestione degli argomenti passati dalla linea di  comando
	int nthread=4;
	int qlen=8;
	int delay=0;
	char opt;
	while( (opt=getopt(argc,argv,"n:q:t:") ) != -1){
		if(opt=='n') nthread=atoi(optarg);
		if(opt=='q') qlen=atoi(optarg);
		if(opt=='t') delay=atoi(optarg);
	}
	if ( nthread< 1 ) termina("numero di thread minore di 1");
	if (qlen< 1 ) termina("dimensione del buffer minore di 1");
	if (delay< 0 ) termina("delay negativo");

	int i=0;

	while(i<3 ){
		if(check_server()) break;
		i++;
		sleep(1);
	}

	if (i==3) termina("il server non risponde");
	char** pcbuff=malloc( sizeof(char*)*qlen );//buffer prod/cons
	sem_t free_slots;
	sem_t data_items;
	xsem_init(&free_slots,0,qlen,QUI);
	xsem_init(&data_items,0,0,QUI);
	pthread_mutex_t pcmux=PTHREAD_MUTEX_INITIALIZER;
	int cind=0;
	int pind=0;

	nthread=  (argc-optind<nthread)?argc-optind:nthread;
	
	pthread_t t[nthread];
	pthread_t sat;//thread signal handler
	th_args a[nthread];
	bool interrupt =false;
	xpthread_create(&sat,NULL,handler,&interrupt,QUI);
	for(int i=0;i<nthread;i++){
		a[i].cind=&cind;
		a[i].data_items=&data_items;
		a[i].free_slots=&free_slots;
		a[i].pcbuff=pcbuff;
		a[i].pcmux=&pcmux;
		a[i].qlen=&qlen;
		xpthread_create(t+i,NULL,wbody,a+i,QUI);
	}
	//invio nome file sul buffer
	while(optind<argc && !interrupt){
		sleep(delay/1000);

		xsem_wait(&free_slots,QUI);
		xpthread_mutex_lock(&pcmux,QUI);
		pcbuff[ pind++ % qlen ]= argv[optind];
		xpthread_mutex_unlock(&pcmux,QUI);
		xsem_post(&data_items,QUI);
		optind++;
	}
	//messaggio di terminazione dei thread worker
	for(int i=0;i<nthread;i++){
		xsem_wait(&free_slots,QUI);
		xpthread_mutex_lock(&pcmux,QUI);
		pcbuff[ pind++ % qlen ]=STOP;
		xpthread_mutex_unlock(&pcmux,QUI);
		xsem_post(&data_items,QUI);
	}
	//attesa terminazinee thread worker
	for(int i=0;i<nthread ;i++){
		xpthread_join(t[i],NULL,QUI);	
	}
	pthread_kill(sat,SIGINT);
	xpthread_join(sat,NULL,QUI);
	//chiusura server
	close_server();
	//deallocazione memoria 
	free(pcbuff);
	xpthread_mutex_destroy(&pcmux,QUI);
	sem_destroy(&free_slots);
	sem_destroy(&data_items);
	return 0;
}