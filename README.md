# client 

## masterworker
Il client fai uso di thread(i _worker_) per poter rendere piu veloce la trasmissione dei dati 
a tale scopo e' stata scelta la tecnica produttori/consumatori dove il thread principale(il _masterworker_) inserisce 
i nomi dei file nel caso arrivi il segnale SIGINT o termini i file da leggere segnala ai thread di terminare con un messaggio
nell buffer produttori/consumatori.
Per accettarsi che il server sia attivo quando intende connettersi viene avviata e chiusa una connessione iniziale col server , verranno fatti 3 tentativi a distanza di un secondo se tutti e tre i tentativi falliscono il client termina 
## worker
il thread avvia una connessione ed invia un intero
- se e' un numero negativo indica che il server deve terminare la connessione
- se e' 0 indica l'inizio di una connessione
nel caso dell'inizio di una connessione  invia prima 4 byte che sono la lunghezza del messaggio da ricevere
viene inviato dopo un intero long di 8 byte che e' la somma calcolata e successivamente il nome del file
per indicare la terminazione della connessione individuale col thread presente nel server invece verra' inviato un intero negativo e successivamente il thread terminera'.
E' stato scelto di ignorare file non presenti anche se specificati come argomento del programma
Per quanto riguarda la gestione di SIGINT e' stato scelto l'uso di un thread in attesa della SIGINT
tramite una sigwait in modo da poter accedere alle variabili condivise col masterworker ed avvisarlo del segnale arrivato.
# server
il server fa uso di multithreading per rendere piu veloce la ricezione dei dati 
riceve un intero di 4 byte dopo aver accettato una connessione
- se e' 0  uno scambio di dati
- altrimenti e' il messaggio di chiusura del server e quindi aspetta che terminino le connessioni 
in corso e successivamente termina

per quanto rigiarda lo scambio di dati passa la connessione ad un thread che si occupa di ricevere i dati nel formato inviato dai worker e terminare se riceve una lunghezza negativa