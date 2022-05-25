#! /usr/bin/env python3

# crea un file contenente n interi 
# mette la sua somma i*a[i] nel nome 

import random, argparse, struct

Description = """
Genera un file con un numero assegnato di interi a 64 bit
Calcola la somma i*a[i] e la appende al nome del file
Con opzione -s genera un file con una somma assegnata"""


# genera file con somma assegnata
# procede in senso inverso
def genera(args):
  a = []
  tot = 0
  for i in range(args.n-1,1,-1):
    val = random.randint(-10000, 10000)
    tot += i*val
    if tot>= 2**63 or tot< -(2**63):
      print("Attenzione! Somma non rappresentabile in un long")
      print("Non genero nessun file. Ridurre il numero di interi")
      return
    a.append(val)
  # generati tutti i numeri tranne i primi 2  
  if args.s:
    val =  args.s - tot
  else:
    val = random.randint(-10000, 10000)
  tot += val
  a.append(val)
  # primo numero che non influisce nella somma 
  val = random.randint(-10000, 10000)
  a.append(val)
  print("Somma:",tot)
  nome  = args.nomeFile + str(tot)
  with open(nome,"wb") as f:
    for i in range(args.n-1,-1,-1):
      f.write(struct.pack("<q",a[i]))


def main():
  parser = argparse.ArgumentParser(description=Description, formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument('-n', help='numero di interi (default 100)', default=100, type=int)  
  parser.add_argument('-s', help='somma (default random)', type=int)  
  parser.add_argument('nomeFile')
  args = parser.parse_args()
  genera(args)

if __name__ == '__main__':
  main()
