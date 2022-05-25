# comunicazione
la comunicazione fra client e server
avviene fra i thread worker del client e i thread del server
quando un thread apre la connessione invia inizialmente il numero 0 per segnalare che vuole iniziare uno scambio di dati al server quindi comincia ad inviare dati nel seguente modo
invia un intero di 4 byte per inidicare la lunghezza in byte del messaggio 
successivamente segue un numero intero di 8 byte che e' la somma calcolata
infine arriva la stringa contente il nome del file 
se il worker intende chiudere la connessione inviera' un numero negativo
in questo caso il trhead lato server terminera
per inviare un segnale di terminazione al server il client aprira' una connessione ma inviera' un numero diverso da 0 ed il server attendere che finiscano di scambiare  dati le connessioni attive e poi termina 
# client 
nel caso in qui un file specificato non esista verra' ignorato

# server
