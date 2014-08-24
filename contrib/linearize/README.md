# Linearize
Construct a linear, no-fork, best version of the blockchain.

## Step 1: Download hash list

   $ ./linearize-hashes.py linearize.cfg > hashlist.txt

Required configuration file settings for linearize-hashes:
* RPC: rpcuser, rpcpassword

Optional config file setting for linearize-hashes:
* RPC: host, port
* Block chain: min_height, max_height

## Step 2: Copy local block data

   $ ./linearize-data.py linearize.cfg

Required configuration file settings:
* "input": bitcoind blocks/ directory containing blkNNNNN.dat
* "hashlist": text file containing list of block hashes, linearized-hashes.py
output.
* "output_file": bootstrap.dat
      or
* "output": output directory for linearized blocks/blkNNNNN.dat output

Optional config file setting for linearize-data:
* "netmagic": network magic number

