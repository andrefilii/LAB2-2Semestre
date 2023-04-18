#include "xerrori.h"

#define QUI __LINE__,__FILE__

#define Buf1_size 10
#define Buf2_size 15

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
  sem_t *empty;
  sem_t *full;
  FILE *infile;  
} dati_t1;

typedef struct {
  int *buffer; 
  int *buffer2; 
  sem_t *empty;
  sem_t *full;
  sem_t *empty2;
  sem_t *full2;
} dati_t2;

typedef struct {
  int *buffer; 
  sem_t *empty;
  sem_t *full;
  FILE *outfile;
} dati_t3;

void *t1_body(void* arg) {
  dati_t1 *a = (dati_t1 *)arg;

  puts(BLUE "T1 partito" RESET);
  int n = -1, ppindex = 0;
  do {
    int e = fscanf(a->infile, "%d", &n);
    if (e != 1) break; // finito il file oppure qualche errore nella lettura
    printf(BLUE "\tNumero letto da file: %d\n" RESET,n);

    // deve essere positivo per passarlo a t2
    if (n < 0) continue;

    xsem_wait(a->empty,QUI);  // se il buffer è pieno attende che si liberi almeno una posizione
    a->buffer[ppindex % Buf1_size] = n;
    ppindex += 1;
    xsem_post(a->full,QUI);  // segnala che c'è almeno un elemento
    puts(BLUE "\tNumero scritto nel buffer" RESET);

  } while (true);

  // segnalo che sono finiti i numeri inserendo -1
  xsem_wait(a->empty,QUI);  // se il buffer è pieno attende che si liberi almeno una posizione
  a->buffer[ppindex % Buf1_size] = -1;
  ppindex += 1;
  xsem_post(a->full,QUI);  // segnala che c'è almeno un elemento
  puts(BLUE "\tSegnale di terminazione scritto nel buffer" RESET);

  puts(BLUE "T1 sta per finire" RESET);
  pthread_exit(NULL);
}

void *t2_body(void* arg) {
  dati_t2* a = (dati_t2*) arg;

  puts(MAGENTA "T2 partito" RESET);
  int n, pcindex = 0, ppindex = 0;
  do {
    xsem_wait(a->full,QUI);  // se il buffer è vuoto aspetto che arrivi almeno un elemento
    n = a->buffer[pcindex % Buf1_size];
    pcindex += 1;
    xsem_post(a->empty,QUI);
    printf(MAGENTA "\tValore letto da buffer: %d\n",n);
    if (n >= 0) {
      // scrivo il doppio nel secondo buffer
      xsem_wait(a->empty2,QUI);  // se il buffer è pieno attende che si liberi almeno una posizione
      a->buffer2[ppindex % Buf2_size] = n*2;
      ppindex += 1;
      xsem_post(a->full2,QUI);  // segnala che c'è almeno un elemento
      puts(MAGENTA "\tDoppio scritto nel buffer" RESET);
    }
  } while (n != -1);

  // segnalo che sono finiti i numeri inserendo -1
  xsem_wait(a->empty2,QUI);  // se il buffer è pieno attende che si liberi almeno una posizione
  a->buffer2[ppindex % Buf2_size] = -1;
  ppindex += 1;
  xsem_post(a->full2,QUI);  // segnala che c'è almeno un elemento
  puts(MAGENTA "\tSegnale di terminazione scritto nel buffer" RESET);

  puts(MAGENTA "T2 sta per finire" RESET);
  pthread_exit(NULL);
}

void *t3_body(void* arg) {
  dati_t3* a = (dati_t3*) arg;

  puts(CYAN "T3 partito" RESET);
  int n, pcindex = 0;
  do {
    xsem_wait(a->full,QUI);  // se il buffer è vuoto aspetto che arrivi almeno un elemento
    n = a->buffer[pcindex % Buf2_size];
    pcindex += 1;
    xsem_post(a->empty,QUI);
    printf(CYAN "\tValore letto da buffer: %d\n",n);
    if (n >= 0) {
      // scrivo in outfile il numero letto da buffer
      fprintf(a->outfile,"%d\n",n);
      puts(CYAN "T3 ha scritto in outfile" RESET);
    }
  } while (n != -1);

  puts(CYAN "T3 sta per finire" RESET);
  pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
  if(argc!=3) {
    fprintf(stderr,"Uso\n\t%s infile outfile\n", argv[0]);
    exit(1);
  }

  // apro i file
  FILE* infile = fopen(argv[1],"r");
  if (infile == NULL) xtermina(RED "Errore apertura infile" RESET, QUI);
  FILE* outfile = fopen(argv[2],"w");
  if (outfile == NULL) xtermina(RED "Errore apertura outfile" RESET, QUI);

  // buffer t1-t2
  int buffer1[Buf1_size];
  sem_t empty1, full1;
  xsem_init(&empty1,0,Buf1_size,QUI);
  xsem_init(&full1,0,0,QUI);

  // buffer t2-t3
  int buffer2[Buf2_size];
  sem_t empty2, full2;
  xsem_init(&empty2,0,Buf2_size,QUI);
  xsem_init(&full2,0,0,QUI);

  // creazione dati da passare a thread

  dati_t1 t1a;
  t1a.buffer = buffer1;
  t1a.empty = &empty1;
  t1a.full = &full1;
  t1a.infile = infile;

  dati_t2 t2a;
  t2a.buffer = buffer1;
  t2a.buffer2 = buffer2;
  t2a.empty = &empty1;
  t2a.full = &full1;
  t2a.empty2 = &empty2;
  t2a.full2 = &full2;

  dati_t3 t3a;
  t3a.buffer = buffer2;
  t3a.empty = &empty2;
  t3a.full = &full2;
  t3a.outfile = outfile;

  // creazione 
  pthread_t t1,t2,t3;
  xpthread_create(&t1,NULL,t1_body,&t1a,QUI);
  xpthread_create(&t2,NULL,t2_body,&t2a,QUI);
  xpthread_create(&t3,NULL,t3_body,&t3a,QUI);

  // join dei thread
  xpthread_join(t1,NULL,__LINE__,__FILE__);
  puts(YELLOW "T1 terminato");
  xpthread_join(t2,NULL,__LINE__,__FILE__);
  puts(YELLOW "T2 terminato");
  xpthread_join(t3,NULL,__LINE__,__FILE__);
  puts(YELLOW "T3 terminato");
  puts(YELLOW "Tutti i thread hanno terminato");

  return 0;
}
