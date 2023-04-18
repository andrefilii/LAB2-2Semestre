#! /usr/bin/env python3

# esempio di server legge due interi e  
# l'elenco dei primi tra questi interi
import sys, struct, socket

# specifica da dove accettare le connessioni
# (valori cambiabili dalla linea di comando)
HOST = "127.0.0.1"  # interfaccia su cui mettersi in ascolto
PORT = 65432  # Port to listen on (non-privileged ports are > 1023) 

# main del server
def main(host=HOST,port=PORT):
  # creiamo il server socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
    try:  
      server.bind((host, port))
      server.listen()
      while True:
        print("In attesa di un client...")
        # mi metto in attesa di una connessione
        conn, addr = server.accept()
        # lavoro con la connessione appena ricevuta
        # durante la gestione il server non accetta nuove connessioni
        gestisci_connessione(conn,addr)
    # questa eccezione permette di chiudere il server
    except KeyboardInterrupt:
      pass
    print('Va bene smetto...')
    # shutdown del server (la close viene fatta dalla with)  
    server.shutdown(socket.SHUT_RDWR)



# gestisci una singola connessione
# con un client
def gestisci_connessione(conn,addr): 
  # L'uso di with serve solo a garantire che
  # conn venga chiusa all'uscita del blocco
  with conn:  
    print(f"Contattato da {addr}")
    # ---- attendo due interi da 32 bit, quindi 8 byte in totale
    data = recv_all(conn,8)
    assert len(data)==8
    # ---- decodifico i due interi dal network byte order
    inizio  = struct.unpack("!i",data[:4])[0]
    fine = struct.unpack("!i",data[4:])[0]
    print(f"Ho ricevuto i valori", inizio,fine)
    # ---- calcolo elenco dei primi
    primi = elenco_primi(inizio, fine)
    # ---- invio risultato in formato preceduto da lunghezza
    conn.sendall(struct.pack("!i",len(primi))) #lunghezza
    for p in primi:
      conn.sendall(struct.pack("!i",p))       # singoli valori 
    print(f"Finito con {addr}")
 


# Riceve esattamente n byte dal socket conn e li restituisce
# il tipo restituto è "bytes": una sequenza immutabile di valori 0-255
# Lancia una eccezione se la connessione viene chiusa prima di aver
# ricevuto tutti i byte
def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    # riceve blocchi di al più 1024 byte
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
    assert bytes_recd == len(chunks)
  return chunks
 

# restituisce lista dei primi in [a,b]
def elenco_primi(a,b):
  ris = []
  for i in range(a,b+1):
    if primo(i):
      ris.append(i);
  return ris


# dato un intero n>0 restituisce True se n e' primo
# False altrimenti
def primo(n):
    assert n>0, "L'input deve essere positivo"
    if n==1:
        return False
    if n==2:
        return True
    if n%2 == 0:
        return False
    assert n>=3 and n%2==1, "C'e' qualcosa che non funziona"
    for i in range(3,n//2,2):
        if n%i==0:
            return False
        if i*i > n:
            break    
    return True


# invoca il main con i parametri passati sulla linea di comando 
if len(sys.argv)==1:
  main()
elif len(sys.argv)==2:
  main(sys.argv[1])
elif len(sys.argv)==3:
  main(sys.argv[1], int(sys.argv[2]))
else:
  print("Uso:\n\t %s [host] [port]" % sys.argv[0])



