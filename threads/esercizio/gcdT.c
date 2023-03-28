#include "xerrori.h"

// macro per indicare la posiine corrente
#define QUI __LINE__,__FILE__

// dimensione buffer produttori-consumatori
#define Buf_size 20

// colori per console log (solo per debug)
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"


typedef struct {
  int *buffer; 
  int *ppindex;
  int *finito;
  int *count;
  pthread_mutex_t *pmutex_buf;
  pthread_mutex_t *pmutex_file;
  pthread_mutex_t *pmutex_finito;
  sem_t *empty;
  sem_t *full;
  FILE *infile;  
} dati_produttori;

typedef struct {
  int *buffer; 
  int *pcindex;
  int *out;
  int *size;
  sem_t *empty;
  sem_t *full;
} dati_consumatore;

// funzione di compare da usare nel qsort
int cmp_int(const void *va, const void *vb)
{
  int a = *(int *)va, b = *(int *) vb;
  return a < b ? -1 : a > b ? +1 : 0;
}

// calcola gcd di due interi >= 0 non entrambi nulli 
int gcd(int a, int b)
{
	assert(a>=0 && b>=0);
	assert(a!=0 || b!=0);
  if(b==0) return a;
  return gcd(b,a%b);
}

void *pbody(void *arg) {
  dati_produttori *a = (dati_produttori *)arg;

  puts(BLUE "Produttore partito" RESET);
  int c1 = -1, c2 = -1;
  do {
    // accedo al file condiviso in mutua esclusione
    xpthread_mutex_lock(a->pmutex_file,QUI);
    int e = fscanf(a->infile, "%d %d", &c1, &c2);
    xpthread_mutex_unlock(a->pmutex_file,QUI);
    if (e != 2) break; // finito il file oppure qualche errore nella lettura
    printf(BLUE "\tCoppia letta da file: %d %d\n" RESET,c1,c2);

    // calcolo del gcd
    int n = gcd(c1,c2);
    printf(BLUE "\tGCD calcolato: %d\n" RESET,n);

    xsem_wait(a->empty,QUI);  // se il buffer è pieno attende che si liberi almeno una posizione
    xpthread_mutex_lock(a->pmutex_buf,QUI);
    a->buffer[*(a->ppindex) % Buf_size] = n;
    *(a->ppindex) += 1;
    xpthread_mutex_unlock(a->pmutex_buf,QUI);
    xsem_post(a->full,QUI);  // segnala che c'è almeno un elemento
    puts(BLUE "\tGCD scritto nel buffer" RESET);

  } while (true);

  // aggiungo questo thread al conto di quelli finiti
  xpthread_mutex_lock(a->pmutex_finito,QUI);
  *(a->finito) += 1;
  xpthread_mutex_unlock(a->pmutex_finito,QUI);

  puts(BLUE "Produttore sta per finire" RESET);
  pthread_exit(NULL);
}

void *cbody(void *arg) {
  dati_consumatore* a = (dati_consumatore*) arg;

  puts(MAGENTA "Consumatore partito" RESET);
  int n, i = 0;
  do {
    xsem_wait(a->full,QUI);  // se il buffer è vuoto aspetto che arrivi almeno un elemento
    n = a->buffer[*(a->pcindex) % Buf_size];
    *(a->pcindex) += 1;
    xsem_post(a->empty,QUI);
    printf(MAGENTA "\tValore letto da buffer: %d\n",n);
    if (n > 0) {
      int dim = *(a->size);
      if (i >= dim) {
        dim *= 2;
        a->out = realloc(a->out,sizeof(int)*dim);
      }
      a->out[i] = n;
      printf(MAGENTA "\t\tValore inserito nell'array: %d\n" RESET,a->out[i]);
      i += 1;
      *(a->size) = dim;
    }
  } while (n != -1);

  a->out = realloc(a->out, sizeof(int)*i);
  *(a->size) = i;

  puts(MAGENTA "Consumatore sta per finire" RESET);
  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  // controlla numero argomenti
  if(argc!=3) {
      printf("Uso: %s file numT\n",argv[0]);
      return 1;
  }

  // numero di thread produttori
  int tp = atoi(argv[2]);
  assert(tp > 0);

  // apro il file contenente le coppie
  FILE* infile = fopen(argv[1],"r");
  if (infile == NULL) xtermina(RED "Errore apertura infile" RESET, QUI);

  // buffer produttori-consumatore
  int buffer[Buf_size];
  int pindex=0, cindex=0, finito=0;
  pthread_mutex_t mpbuf = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t mpfile = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t mpfinito = PTHREAD_MUTEX_INITIALIZER;
  sem_t empty, full;
  xsem_init(&empty,0,Buf_size,QUI);
  xsem_init(&full,0,0,QUI);

  // dati per i thread
  dati_produttori ap[tp];
  // id thread produttori
  pthread_t prod[tp];

  for(int i=0;i<tp;i++) {
    // faccio partire il thread i
    ap[i].buffer = buffer;
    ap[i].ppindex = &pindex;
    ap[i].finito = &finito;
    ap[i].pmutex_buf = &mpbuf;
    ap[i].pmutex_file = &mpfile;
    ap[i].pmutex_finito = &mpfinito;
    ap[i].empty = &empty;
    ap[i].full = &full;
    ap[i].infile = infile;
    xpthread_create(&prod[i],NULL,pbody,ap+i,QUI);
  }
  puts(YELLOW "Thread produttori creati");

  // creazione thread consumatore
  pthread_t cons;
  dati_consumatore ac;
  int size = 20;
  int* out = malloc(sizeof(int) * size);
  if (out == NULL) xtermina(RED "Erorre malloc" RESET, QUI);
  ac.buffer = buffer;
  ac.empty = &empty;
  ac.full = &full;
  ac.pcindex = &cindex;
  ac.out = out;
  ac.size = &size;
  xpthread_create(&cons,NULL,cbody,&ac,QUI);
  puts(YELLOW "Thread consumatore creato");
  
  // join dei thread produttori
  for(int i=0;i<tp;i++) {
    // aspetta che l'i-esimo thread prod abbia finito
    xpthread_join(prod[i],NULL,__LINE__,__FILE__);
  }
  puts(YELLOW "Tutti i produttori hanno terminato");

  // aspetto che ci sia posto nel buffer, inserisco il valore di controllo -1
  //  e dico al consumatore che c'è almeno un altro dato da leggere per
  //  farlo terminare
  for(int i=0;i<tp;i++) {
    xsem_wait(&empty,__LINE__,__FILE__);
    // metto nel buffer -1 per far terminare eventuali thread ancora funzionanti
    buffer[pindex++ % Buf_size]= -1;
    xsem_post(&full,__LINE__,__FILE__);
  }
  puts(YELLOW "Valori di terminazione scritti" RESET);

  // aspetto che il consumatore abbia terminato
  xpthread_join(cons,NULL,__LINE__,__FILE__);
  // faccio puntare out al giusto array visto che con la realloc nel consumatore
  //  è stato distrutto
  out = ac.out;

  printf(YELLOW "%d su %d thread produttori hanno finito. GCD trovati: %d\n" RESET,finito,tp,size);

  // ordinamento e stampa di out
  qsort(out,size,sizeof(int),cmp_int);

  puts (YELLOW "--- ARRAY ORDINATO ---" RESET);
  for (int i = 0; i < size; i++)
  {
    printf(YELLOW "%d)\t%d\n" RESET, i+1, out[i]);
  }

  // libero memoria ecc
  xsem_destroy(&empty,QUI);
  xsem_destroy(&full,QUI);
  xpthread_mutex_destroy(&mpbuf,QUI);
  xpthread_mutex_destroy(&mpfile,QUI);
  xpthread_mutex_destroy(&mpfinito,QUI);
  free(out);
  fclose(infile); 

  puts("");
  return 0;
}

