#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define HOST "127.0.0.1"
#define PORT 1026


typedef struct {
	int* pind;
	int* cind;
	pthread_mutex_t * pcmux;
	char ** pcbuff;
	sem_t * free_slots;
	sem_t * data_items;
	int * qlen;
} th_args;

void* inthandler(void*args){
	th_args* d=(th_args*) args;
	sigset_t mask;
	sigfillset(&mask);
	pthread_sigmask(SIG_BLOCK,&mask,NULL);
	sigemptyset(&mask);
	sigaddset(&mask,SIGINT);
	pthread_sigmask(SIG_UNBLOCK,&mask,NULL);
	int s;
	sigwait(&mask,&s);
	for(int i=0;i<*d->qlen;i++){
		xsem_wait(d->free_slots,QUI);
		xpthread_mutex_lock(d->pcmux,QUI);
		d->pcbuff[ *d->pind % *d->qlen ]="END";
		*d->pind += 1;
		xpthread_mutex_unlock(d->pcmux,QUI);
		xsem_post(d->data_items,QUI);
	}
	return NULL;
}

void* wbody(void* args){
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGINT);
	pthread_sigmask(SIG_UNBLOCK,&mask,NULL);
	struct sockaddr_in addr;
	int socketfd;
	socketfd = socket(AF_INET,SOCK_STREAM,0);
	if(socketfd<0)
		termina("errora creazione socket");
	addr.sin_family= AF_INET;
	addr.sin_port=htons(PORT);
	addr.sin_addr.s_addr=inet_addr(HOST);
	if( connect(socketfd,(struct sockaddr*)&addr,sizeof(addr))<0 )
		termina("errore apertura connessione");
	int tmp;
	th_args* d=(th_args*)args;
	char* name;
	tmp=htonl(0);
	writen(socketfd,&tmp,sizeof(int));
	while(true){
		xsem_wait(d->data_items,QUI);
		xpthread_mutex_lock(d->pcmux,QUI);
		name=d->pcbuff[ *d->cind % *d->qlen ];
		*d->cind += 1;
		xpthread_mutex_unlock(d->pcmux,QUI);
		xsem_post(d->free_slots,QUI);
		
		printf("posso scrivere ho trovato %s\n",name);

		if(strcmp("END",name)==0){
			tmp=htonl(11);
			writen(socketfd,&tmp,sizeof(int));
			tmp=htonl(0);
			writen(socketfd,&tmp,sizeof(int));
			writen(socketfd,&tmp,sizeof(int));
			writen(socketfd,name,sizeof(char)*3);
			printf("ho trovato END\n" );
			break;
		}
		FILE* f=fopen(name,"rb");
		long sum=0;
		long n=0;
		int i=0;
		while( fread(&n,sizeof(long),1,f) == 1 ){
			sum += i* n;
			i++;
		}
		int len;
		len=strlen(name)+sizeof(long)+1;
		printf("invio %d bytes somma:%ld nome:%s\n",len,sum,name);

		tmp=htonl(len);
		writen(socketfd,&tmp,sizeof(int));//lunghezza messaggio
		int sum1=sum>>32;
		int sum2=(int)sum;
		tmp=htonl(sum1);
		writen(socketfd,&tmp,sizeof(int));//valore calcolato
		tmp=htonl(sum2);
		writen(socketfd,&tmp,sizeof(int));//valore calcolato
		writen(socketfd,name,sizeof(char)*len);//nome file
		fclose(f);
	}
	close(socketfd);
	return NULL;
}

void close_server(){
	struct sockaddr_in addr;
	int socketfd;
	socketfd = socket(AF_INET,SOCK_STREAM,0);
	if(socketfd<0)
		termina("errora creazione socket");
	addr.sin_family= AF_INET;
	addr.sin_port=htons(PORT);
	addr.sin_addr.s_addr=inet_addr(HOST);
	if( connect(socketfd,(struct sockaddr*)&addr,sizeof(addr))<0 )
		termina("errore apertura connessione");
	int tmp=htonl(-1);
	writen(socketfd,&tmp,sizeof(int));
	close(socketfd);
}

int main(int argc, char *argv[])
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGINT);
	pthread_sigmask(SIG_UNBLOCK,&mask,NULL);
  // controlla numero argomenti
	

  if(argc<2) {
      printf("Uso: %s file [file ...] \n",argv[0]);
      return 1;
  }
	int nthread=4;
	int qlen=8;
	int delay=0;
	char opt;
	while( (opt=getopt(argc,argv,"n:q:t:") ) != -1){
		if(opt=='n') nthread=atoi(optarg);
		if(opt=='q') qlen=atoi(optarg);
		if(opt=='t') delay=atoi(optarg);
	}
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
	th_args a[nthread];
	
	for(int i=0;i<nthread;i++){
		a[i].cind=&cind;
		a[i].data_items=&data_items;
		a[i].free_slots=&free_slots;
		a[i].pcbuff=pcbuff;
		a[i].pcmux=&pcmux;
		a[i].qlen=&qlen;
		xpthread_create(t+i,NULL,wbody,a+i,QUI);
	}
	printf("%d\n",argc);
	while(optind<argc){
		sleep(delay);
		xsem_wait(&free_slots,QUI);
		xpthread_mutex_lock(&pcmux,QUI);
		pcbuff[ pind++ % qlen ]= argv[optind];
		xpthread_mutex_unlock(&pcmux,QUI);
		xsem_post(&data_items,QUI);
		printf("ho inserito dei dati\n");
		optind++;
	}
	for(int i=0;i<qlen;i++){
		xsem_wait(&free_slots,QUI);
		xpthread_mutex_lock(&pcmux,QUI);
		pcbuff[ pind++ % qlen ]= "END";
		xpthread_mutex_unlock(&pcmux,QUI);
		xsem_post(&data_items,QUI);
	}
	for(int i=0;i<nthread ;i++){
		xpthread_join(t[i],NULL,QUI);	
	}
	close_server();
	return 0;
}