# client 
Il client fai uso di multithreading per poter rendere piu veloce la trasmissione dei dati , per rendere piu semplice la comunicazione  e' stata adottata la tecnica  produttori consumatori,
il thread principale (_masterworker_) e' il produttore mentre i thread (_worker_) sono i lettori.

Questo sistema consente di inviare ad ogni thread un file alla volta assicurando tramite  l'uso dei semafori `data_items` e `free_slots` l'inserimento di dati quando ci slot che sono disponibili perche' gia letti o non utilizzati e tramite la mutex `pcmux` consente l'accesso ad un solo thread alla volta per evitare sovrascritture indesiderate da parte del masterworker o la lettura dello stesso file da due worker diversi nel buffer condiviso `pcbuff`.
# masterworker
Per controllare se il server e' attivo all'avvio dell'applicazine vengono effettuati tre tentivi per aprire e chiudere una connessione col server tramite la funzione `check_server()` a distanza di un secondo uno dall'altro questo serve perche e' possibile che il server non riesca a ricevere correttamnete l'inizializzazione attraverso la rete in questo modo si evita di chiudere il programma nonostante il server sia attivo e consentono di determinare se e' effettivamente attivo.
# trasmissione dati
per poter garantire il corretto invio dei dati e la terminazione del server i dati vengono trasmessi dai worker nel seguente formato un intero di 4 byte
- se e' 0 indica l'inizio dello scambio di dati nel seguente formato:
    1. lunghezza del messaggio (4 byte)
    2. somma calcolata (8 byte)
    3. nome del file (lunghezza messaggio - 8)
    
    se la lunghezza ricevuta e' negativa indica la chiusura della connessione iniziata dal thread
- altrimenti verra interpretato come messaggio di chiusura del server

questo sistema consente di distinguere tramite i primi 4 byte se intendo scambiare dati oppure no mentre per quanto riguarda i dati con la lunghezza negativa si puo' segnalare ai thread del server l'intenzione di terminare oppure continuare a scambiare dati fra i thread.
# server
 Legge i primi 4 byte per poter decidere se si tratta di un inizio di scambio dati o il messaggio di chiusura in caso sia uno scambio dati affida la connessione appena creata ad un thread che si occupera di ricevere i dati in accordo col formato sopra descritto questo meccanismo consente di gestire piu velocemente richieste da client multipli.
# gestione segnale di interruzione
E'stato scelto l'utilizzo di un apposito thred che attende il segnale `SIGINT` per poter convidivedere una variabile con il masterworker in modo da segnalargli l'arrivo di SIGINT per fargli smettere di invaire dati sul buffer e cominciare a far termianre i thread ed inviare il messaggio di chiusura al server.