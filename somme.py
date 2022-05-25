
#! /usr/bin/env python3

# calcola la chesum i*a[i] per i file
# di long passati sullalinea di comando

import sys, struct, argparse 

Description = """
Calcola la somma per i file passati sulla linea dei comando
Accetta anche i wildcard nei nomi
Con opzione -s ordina l'output per somma crescente"""
  

def somma(args):
  risultati = []
  for nfile in sorted(args.files):
    somma = 0
    with open(nfile,"rb") as f:
      i=0
      while True:
        data = f.read(8)
        if len(data)==0: break
        v = struct.unpack("<q",data)[0]
        somma += i*v
        i += 1
      risultati.append((somma, nfile))
      if somma>= 2**63 or somma < - (2**63):
        print("Attenzione! Somma maggiore di 2^63: non rappresentabile in un long")
  # eventualmente ordina e poi stampa 
  if args.s:
    risultati.sort()
  for (somma,nfile) in risultati:
    print("%12s" % somma,nfile)
  return    


def main():
  parser = argparse.ArgumentParser(description=Description, formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument('-s', help='ordina per somma', action="store_true")  
  parser.add_argument('files', nargs='+')
  args = parser.parse_args()
  somma(args)

if __name__ == '__main__':
  main()
