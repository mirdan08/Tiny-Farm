#!/usr/bin/env python3

import sys,struct,socket,threading

HOST = "127.0.0.1"
PORT = 1033

def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks
 
#riceve somma e lunghezza 
#nel caso in cui sia negativa termina il thread e la connessione
def receive_file(conn,addr):
	with conn:
		while True:
			try:
				data=recv_all(conn,4)#leggo lunghezza file
				length=struct.unpack("!i",data)[0]
				if length < 0:
					break
				data=recv_all(conn,length)
				somma=struct.unpack("!q",data[0:8])[0]
				nome=struct.unpack(f"!{length-8}s",data[8:])[0].decode()
				print(f"\t{somma:10}\t{nome}")
			except RuntimeError:
				break
			except UnicodeError:
				break
	
def main(host=HOST,port=PORT):
	#apertura e configurazione server
	with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as s:
		try:
			s.bind((host,port))
		except OSError:
			print("porta non disponibile riprovare piu tardi")
			return None
		s.listen()
		connections=[]
		while True:
			conn,addr=s.accept()
			data=recv_all(conn,4)#leggo lunghezza file
			msg=struct.unpack("!i",data)[0]
			if msg == 0:
				t= threading.Thread(target=receive_file,args=(conn,addr))
				connections.append(t)
				t.start()
			else:
				#attende terminazione thread in caso di messaggio di terminazione
				for t in connections:
					t.join()
				break
		s.shutdown(socket.SHUT_RDWR)

main()