#include "xerrori.h"

// costruzione di una tabella di primi con thread multipli

// questo programma è stato ottenuto partendo da
// comtaprimi2.c aggiungendo la gesxtione di una tabella

/*
  I mutex (mutual exclusion) permettono, come dice il nome, di ottenere l'esecuzione sequenziale
  ed esclusiva (quindi atomica) di parti di codice condiviso. Questo serve con l'uso della memoria
  condivisa per evitare sovrascritture di variabili condivise (come in questo caso la var. messi o tabella).

  Si comportano come semafori binare: possono avere valore 1/0 (lock/unlock) ma la differenza è che mentre i semafori
  possono far fare la post() a chiunque, i mutex possono essere UNLOCKATI solo da thread che li aveva precedentemente lockati.

*/

//Prototipi
bool primo(int n);

// struct che uso per passare argomenti ai thread
typedef struct {
  int start;            // intervallo dove cercare i primo 
  int end;              // parametri di input
  int somma_parziale;   // parametro di output
  int* tabella;         // puntatore alla tabella dove inserire i primi
  int* messi;            // contatore del totale dei primi messi
  pthread_mutex_t *mutex_tab; // mutex condiviso fra tutti i thread per aggiornare tabella e "messi"
} dati;

// funzione passata a pthred_create
void *tbody(void *v) {
  dati *d = (dati *) v;
  d->somma_parziale = 0;
  // cerco i primi nell'intervallo assegnato
  for(int j=d->start;j<d->end;j++)
      if(primo(j)){
        d->somma_parziale += j;

        // il thread blocca il mutex per avere le operazioni fatte in modo ATOMICO
        // Differenza con il semaforo: IL MUTEX PUO ESSERE SBLOCCATO SOLO DAL THREAD CHE HA FATTO LA LOCK
        // Lavorando sulle stesse variabili il mutex sarà uguale per tutti i thread
        // Nota su xpthread_...() : fa il controllo sugli errori:
        //  0 -> tutto bene
        //  altro -> errore
        xpthread_mutex_lock(d->mutex_tab, __LINE__, __FILE__);
        d->tabella[*(d->messi)] = j;
        *(d->messi) += 1;
        xpthread_mutex_unlock(d->mutex_tab, __LINE__, __FILE__);
      }
  fprintf(stderr, "Il thread che partiva da %d ha terminato\n", d->start);
  pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
  if(argc!=3) {
    fprintf(stderr,"Uso\n\t%s m num_threads\n", argv[0]);
    exit(1);
  }
  // conversione input
  int m= atoi(argv[1]);
  if(m<1) termina("limite primi non valido");
  int p= atoi(argv[2]);
  if(p<=0) termina("numero di thread non valido");

  // creazione thread ausiliari
  pthread_t t[p];   // array di p indentificatori di thread 
  dati d[p];        // array di p struct che passerò allle p thread

  int *tabella = malloc(m*sizeof(int));
  if(tabella==NULL) xtermina("Allocazione fallita", __LINE__, __FILE__);
  int messi = 0;

  // Creazione del mutex UNICO per tutti i thread
  //  Viene inizializzato con una macro che richiama la init() con parametri di default
  pthread_mutex_t mtabella = PTHREAD_MUTEX_INITIALIZER;

  for(int i=0; i<p; i++) {
    int n = m/p;  // quanti numeri verifica ogni figlio + o - 
    d[i].start = n*i; // inizio range figlio i
    d[i].end = (i==p-1) ? m : n*(i+1);
    d[i].mutex_tab = &mtabella;
    d[i].tabella = tabella;
    d[i].messi = &messi; // serve il puntatore perchè la variabile è condivisa fra tutti (è il conto totale dei primi messi)
    xpthread_create(&t[i], NULL, &tbody, &d[i],__LINE__, __FILE__); 
  }
  // attendo che i thread abbiano finito
  int somma = 0;        // variabile dove accumulo il numero di primi
  for(int i=0;i<p;i++) {
    xpthread_join(t[i],NULL,__LINE__, __FILE__);
    somma += d[i].somma_parziale;
  }
  // stampa tabella
  for(int i=0;i<messi;i++)  printf("%8d",tabella[i]);
  printf("\nPrimi in tabella: %d\n",messi);
  // restituisce il numero di primi
  printf("Somma primi tra 1 e %d (escluso): %d\n",m,somma);
  return 0;
}




// restituisce true/false a seconda che n sia primo o composto
bool primo(int n)
{
  if(n<2) return false;
  if(n%2==0) {
    if(n==2)  return true;
    else return false; 
  }
  for (int i=3; i*i<=n; i += 2) 
    if(n%i==0) return false;
  return true;
}

