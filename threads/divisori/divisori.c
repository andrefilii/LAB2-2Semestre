/*
 * Esempio semplice paradigma produttore consumatori
 * Il produttore legge interi da un file e i consumatori calcolano 
 * la somma dei primi
 * 
 * Usare il numeri.py per generare lunghi elenchi di interi positivi su cui testare il programma
 * 
 * Programma di esempio del paradigma 1 producer 1 consumer
 * i dati letti dal file vengono messi su un buffer in cui il producer scrive 
 * e i consumer leggono. In principio il buffer va bene di qualsiasi dimensione: 
 * piu' e' grande maggiore e' il lavoro pronto da svolgere nel caso
 * il produttore rimanga bloccato (ad esempio a leggere dal disco)
 * 
 * */
#include "../xerrori.h"

#define Buf_size 10


// funzione per stabilire se n e' primo  
bool primo(int n)
{
  if(n<2) return false;
  if(n%2==0) return (n==2);
  for (int i=3; i*i<=n; i += 2)
      if(n%i==0) return false;
  return true;
}

// struct contenente i parametri di input e output di ogni thread 
typedef struct {
  int quanti;   // output
  long somma;   // output
  int *buffer; 
  int *pcindex; // variabile condivisa dai thread per scorrere il buffer
  pthread_mutex_t* pmutex_buffer; // mutex condiviso fra consumatori per accesso al buffer
  pthread_mutex_t* pmutex_file; // mutex per acceso al file
  sem_t *sem_free_slots;
  sem_t *sem_data_items;  
  FILE *outfile;
} dati_consumatori;



// funzione eseguita dai thread consumer
void *cbody(void *arg)
{  
  dati_consumatori *a = (dati_consumatori *)arg; 
  a->quanti = 0;
  a->somma = 0;
  int n;
  do {

    // accesso al buffer
    xsem_wait(a->sem_data_items,__LINE__,__FILE__);
    xpthread_mutex_lock(a->pmutex_buffer,__LINE__,__FILE__);
    n = a->buffer[*(a->pcindex) % Buf_size];
    *(a->pcindex) +=1;
    xpthread_mutex_unlock(a->pmutex_buffer,__LINE__,__FILE__);
    xsem_post(a->sem_free_slots,__LINE__,__FILE__);
    
    int div = divisori(n);

    // scrittura su file
    xpthread_mutex_lock(a->pmutex_file,__LINE__,__FILE__);
    fprintf(a->outfile, "%d %d\n", n, div);
    xpthread_mutex_unlock(a->pmutex_file,__LINE__,__FILE__);
  } while(n!= -1);
  pthread_exit(NULL); 
}

void *pbody(void *arg) {

}


int main(int argc, char *argv[])
{
  // leggi input
  if(argc!=2) {
    printf("Uso\n\t%s file\n", argv[0]);
    exit(1);
  }
  int p = 1;
  assert(p>=0);
  int tot_primi = 0;
  long tot_somma = 0;
  int e,n;    
  // threads related
  int buffer[Buf_size];
  int pindex = 0;
  int cindex = 0;
  pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
  pthread_t t[p];
  dati_consumatori a[p];
  sem_t sem_free_slots, sem_data_items;
  xsem_init(&sem_free_slots,0,Buf_size,__LINE__,__FILE__);
  xsem_init(&sem_data_items,0,0,__LINE__,__FILE__);
  for(int i=0;i<p;i++) {
    // faccio partire il thread i
    a[i].buffer = buffer;
    a[i].pcindex = &cindex; // variabile condivisa
    a[i].pmutex_buffer = &mu;
    a[i].sem_data_items = &sem_data_items;
    a[i].sem_free_slots = &sem_free_slots;
    xpthread_create(&t[i],NULL,cbody,a+i,__LINE__,__FILE__);
  }


  // leggi file 
  FILE *f = fopen(argv[1],"r");
  if(f==NULL) {perror("Errore apertura file"); return 1;}
  while(true) {
    e = fscanf(f,"%d", &n);
    if(e!=1) break; // se il valore e' letto correttamente e==1
    assert(n>0);    // i valori del file devono essere positivi
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    buffer[pindex++ % Buf_size]= n;
    xsem_post(&sem_data_items,__LINE__,__FILE__);
  }
  // terminazione threads
  for(int i=0;i<p;i++) {
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    // metto nel buffer -1 per far terminare eventuali thread ancora funzionanti
    buffer[pindex++ % Buf_size]= -1;
    xsem_post(&sem_data_items,__LINE__,__FILE__);
  }
  // join dei thread e calcolo risulato
  for(int i=0;i<p;i++) {
    // aspetta che l'i-esimo thread abbia finito
    xpthread_join(t[i],NULL,__LINE__,__FILE__);
    tot_primi += a[i].quanti;
    tot_somma += a[i].somma;
  }
  xpthread_mutex_destroy(&mu,__LINE__,__FILE__);
  fprintf(stderr,"Trovati %d primi con somma %ld\n",tot_primi,tot_somma);
  return 0;
}

