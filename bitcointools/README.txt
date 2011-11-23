----- dbdump.py -----
Run    dbdump.py --help    for usage.  Database files are opened read-only, but
you might want to backup your Bitcoin wallet.dat file just in case.

You must quit Bitcoin before reading the transactions, blocks, or address database files.

Requires the pycrypto library from  http://www.dlitz.net/software/pycrypto/
to translate public keys into human-friendly Bitcoin addresses.

Examples:

Print out  wallet keys and transactions:
  dbdump.py --wallet --wallet-tx

Print out the "genesis block" (the very first block in the proof-of-work block chain):
  dbdump.py --block=000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f

Print out one of the transactions from my wallet:
  dbdump.py --transaction=c6e1bf883bceef0aa05113e189982055d9ba7212ddfc879798616a0d0828c98c
  dbdump.py --transaction=c6e1...c98c

Print out all 'received' transactions that aren't yet spent:
  dbdump.py --wallet-tx-filter='fromMe:False.*spent:False'

Print out all blocks involving transactions to the Bitcoin Faucet:
  dbdump.py --search-blocks=15VjRaDX9zpbA8LVnbrCAFzrVzN7ixHNsC

There's a special search term to look for non-standard transactions:
  dbdump.py --search-blocks=NONSTANDARD_CSCRIPTS

----- statistics.py -----
Scan all the transactions in the block chain and dump out a .csv file that shows transaction volume per month.

----- fixwallet.py -----
Half-baked utility that reads a wallet.dat and writes out a new wallet.dat.

Only half-baked because to be really useful I'd have to write serialize routines to re-pack data after modifying it...

----- jsonToCSV.py -----
Read JSON list-of-objects from standard input, writes CSV file to standard output.
Useful for converting bitcoind's listtransactions output to CSV that can be
imported into a spreadsheet.
