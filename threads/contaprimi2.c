#include "xerrori.h"

/* VERSIONE CONTAPRIMI SENZA SEMAFORI USANDO SOMME PARZIALI */

// conteggio dei primi con thread multipli

// NOTA: questo programma ha solo interesse didattico!
// dal punto di vista delle prestazionei è una pessima idea 
// utilizzare una sola variabile condivisa a cui i thread 
// accedono continuamente. 

// Inoltre in questo caso i thread ausiliari non fanno altre 
// operazioni dopo il calcolo dei primi quindi il meccanismo 
// della join si potrebbe utilizzare per comunicare
// la terminazione al thread principale 

//Prototipi
bool primo(int n);

// struct che uso per passare argomenti ai thread
typedef struct {
	int start;   // parametro di input, intervallo dove cercare i primo 
	int end;     // parametro di input
  int somma_parziale; // parametro di output
} dati;

// funzione passata a pthred_create
// Come ogni funzione può accedere solo a variabili globali e a quelle nel suo scope
//  questa cosa può essere sfruttata per evitare che i thread vadano in conflitto 
//  con l'uso della memoria
void *tbody(void *v) {
	dati *d = (dati *) v;
  // variabile locale del thread. Ogni thread ha uno stack separato.
  //  quindi posso usarla senza dover usare semafori
  int somma = 0;
	// cerco i primi nell'intervallo assegnato
	for(int j=d->start;j<d->end;j++) {
        if(primo(j)) somma++;
	}
  fprintf(stderr,"Il thread che partiva da %d ha terminato\n",d->start);
  d->somma_parziale = somma;
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

  // serve un array di dati perchè i thread condividono la memoria e non la copiano,
  //  quindi ogni thread avrà una posizione nell'array ma potrebbe teoricamente accedere
  //  tutto il resto. Per questo alla funzione passo solo il puntatore corrispondente di
  //  modo che possa accedere solo a quella
	dati d[p];        // array di p struct che passerò allle p thread
	int somma = 0;        // variabile dove accumulo il numero di primi
  for(int i=0; i<p; i++) {
    int n = m/p;  // quanti numeri verifica ogni figlio + o - 
    d[i].start = n*i; // inizio range figlio i
    d[i].end = (i==p-1) ? m : n*(i+1);
    // salvo nell'array t gli identificatori dei thread appena creati. Servirà poi nelle join
    if(pthread_create(&t[i], NULL, &tbody, &d[i])!=0)
			termina("Errore creazione thread");
  }
  // attendo che i thread abbiano finito
  for(int i=0; i<p; i++) {
    // la funzione è BLOCCANTE, si ferma finchè il t[i] thread non fa la pthread_exit()
    xpthread_join(t[i], NULL, __LINE__, __FILE__);
    // una volta che sono sicuro che l'i-esimo t. abbia finito
    // posso aggiungere il suo risultato al totale
    somma += d[i].somma_parziale;
  }
    
  // restituisce il risultato 
  printf("Numero primi tra 1 e %d (escluso): %d\n",m,somma);
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

