=====================================   
Bitcoin Core integration/staging tree   
License  
-------  
Bitcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more  
information or see https://opensource.org/licenses/MIT.  
https://bitcoincore.org/en/download/.  
  
  Fork Dev Branch Progress:  
   - v0.0.3 - Improvements to increase the security and performance of node
   
Fork Updates:  
   - v0.0.2 - Added to build.sh file, Base16 with preconfig of bitcoin.conf  
                with all options on file to improve the user access  
                all describe on: https://bitcoin.org/en/full-node#configuration-file          
  
   - v0.0.1 - Added build.sh to compile and make the bitcoin-core/node  
     
       Fork Dev canceled Progress:  
   - v0.0.3 - ~Adding Consensus of Maturity to transactions of output.~
                ~the transaction only procced if the units of bitcoin in output~
                 ~already have reached in your maturity of lasts N blocks~  
                 (Already exist without necessity of change the code  
                 only need that you insert the rule over bitcoin.conf to your node work of accord this)  
                 
=====================================  
    
Explaining Bitcoin.conf  
testnet=0: This setting specifies whether to run on the testnet (1) or the main network (0).  
server=1: This setting enables the Bitcoin Core node to act as a server and accept incoming connections.  
rpcbind=127.0.0.1: This setting specifies the IP address that the Bitcoin Core node will listen on for incoming JSON-RPC API requests.  
rpcallowip=127.0.0.1: This setting specifies which IP addresses are allowed to make JSON-RPC API requests to the Bitcoin Core node. In this case, only the local machine is allowed.  
rpcuser=<your rpc username>: This setting specifies the username for accessing the JSON-RPC API.  
rpcpassword=<your rpc password>: This setting specifies the password for accessing the JSON-RPC API.  
prune=550: This setting specifies the maximum amount of disk space that the Bitcoin Core node will use for storing blockchain data, in megabytes. In this case, pruning is enabled and the node will store at most 550 MB of data.  
dbcache=1024: This setting specifies the amount of memory that the Bitcoin Core node will use for caching database entries, in megabytes.  
maxmempool=500: This setting specifies the maximum size of the transaction memory pool, in megabytes.  
maxconnections=40: This setting specifies the maximum number of peer connections that the Bitcoin Core node will establish.  
banscore=10000: This setting specifies the score threshold for banning misbehaving peers.  
bantime=604800: This setting specifies the duration of a ban, in seconds.  
whitelist=127.0.0.1: This setting specifies a list of IP addresses that the Bitcoin Core node will never ban.  
disablewallet=1: This setting disables the wallet functionality of the Bitcoin Core node.  
listenonion=0: This setting specifies whether the Bitcoin Core node should listen for incoming Tor connections.  
onlynet=ipv4: This setting specifies which network protocols to use.  
proxy=127.0.0.1:9050: This setting specifies the SOCKS5 proxy address for outgoing connections.  
dbcrashverify=1: This setting enables verification of database entries after a crash.  
walletdisableprivkeys=1: This setting disables private key operations on the wallet.  
maxsigcachesize=1: This setting specifies the maximum signature cache size, in megabytes.  
maxscriptvalidationthreads=1: This setting specifies the maximum number of script validation threads.  
maxstackmemoryusageconsensus=1000000: This setting specifies the maximum stack memory usage, in bytes.  
checkpoints=0: This setting disables checkpoints.  
maxuploadtarget=5000: This setting specifies the maximum upload bandwidth, in kilobytes per second.  
debug=0: This setting disables debug logging.  
logips=1: This setting enables logging of IP addresses.  
logtimestamps=1: This setting enables logging of timestamps.  
upnp=0: This setting disables UP  
