Running Bitcoind
---------------------
Bitcoind is a headless daemon, and also bundles a testing tool for the same daemon. It provides a JSON-RPC interface, allowing it to be controlled locally or remotely which makes it useful for integration with other software or in larger payment systems. Various commands are made available by the [API](bitcoind-api.md).

To use locally, first start the program in daemon mode:

	bitcoind -daemon

Then you can use the same program to execute API commands, e.g.:

	bitcoind getinfo
	bitcoind listtransactions

To stop the bitcoin daemon, execute:

	bitcoind stop

Bitcoind API
---------------------

Give Bitcoin (or bitcoind) the `-?` or `–-help` argument and it will print out a list of the most commonly used command-line arguments and then exit: 

	-?                     This help message
	-conf=<file>           Specify configuration file (default: bitcoin.conf)
	-pid=<file>            Specify pid file (default: bitcoind.pid)
	-gen                   Generate coins (default: 0)
	-datadir=<dir>         Specify data directory
	-dbcache=<n>           Set database cache size in megabytes (default: 25)
	-timeout=<n>           Specify connection timeout in milliseconds (default: 5000)
	-proxy=<ip:port>       Connect through socks proxy
	-socks=<n>             Select the version of socks proxy to use (4-5, default: 5)
	-tor=<ip:port>         Use proxy to reach tor hidden services (default: same as -proxy)
	-dns                   Allow DNS lookups for -addnode, -seednode and -connect
	-port=<port>           Listen for connections on <port> (default: 8333 or testnet: 18333)
	-maxconnections=<n>    Maintain at most <n> connections to peers (default: 125)
	-addnode=<ip>          Add a node to connect to and attempt to keep the connection open
	-connect=<ip>          Connect only to the specified node(s)
	-seednode=<ip>         Connect to a node to retrieve peer addresses, and disconnect
	-externalip=<ip>       Specify your own public address
	-onlynet=<net>         Only connect to nodes in network <net> (IPv4, IPv6 or Tor)
	-discover              Discover own IP address (default: 1 when listening and no -externalip)
	-checkpoints           Only accept block chain matching built-in checkpoints (default: 1)
	-listen                Accept connections from outside (default: 1 if no -proxy or -connect)
	-bind=<addr>           Bind to given address and always listen on it. Use [host]:port notation for IPv6
	-dnsseed               Find peers using DNS lookup (default: 1 unless -connect)
	-banscore=<n>          Threshold for disconnecting misbehaving peers (default: 100)
	-bantime=<n>           Number of seconds to keep misbehaving peers from reconnecting (default: 86400)
	-maxreceivebuffer=<n>  Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)
	-maxsendbuffer=<n>     Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)
	-upnp                  Use UPnP to map the listening port (default: 1 when listening)
	-paytxfee=<amt>        Fee per KB to add to transactions you send
	-server                Accept command line and JSON-RPC commands
	-testnet               Use the test network
	-debug                 Output extra debugging information. Implies all other -debug* options
	-debugnet              Output extra network debugging information
	-logtimestamps         Prepend debug output with timestamp
	-shrinkdebugfile       Shrink debug.log file on client startup (default: 1 when no -debug)
	-printtoconsole        Send trace/debug info to console instead of debug.log file
	-printtodebugger       Send trace/debug info to debugger
	-rpcuser=<user>        Username for JSON-RPC connections
	-rpcpassword=<pw>      Password for JSON-RPC connections
	-rpcport=<port>        Listen for JSON-RPC connections on <port> (default: 8332 or testnet: 18332)
	-rpcallowip=<ip>       Allow JSON-RPC connections from specified IP address
	-rpcthreads=<n>        Set the number of threads to service RPC calls (default: 4)
	-blocknotify=<cmd>     Execute command when the best block changes (%s in cmd is replaced by block hash)
	-walletnotify=<cmd>    Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)
	-alertnotify=<cmd>     Execute command when a relevant alert is received (%s in cmd is replaced by message)
	-upgradewallet         Upgrade wallet to latest format
	-keypool=<n>           Set key pool size to <n> (default: 100)
	-rescan                Rescan the block chain for missing wallet transactions
	-salvagewallet         Attempt to recover private keys from a corrupt wallet.dat
	-checkblocks=<n>       How many blocks to check at startup (default: 288, 0 = all)
	-checklevel=<n>        How thorough the block verification is (0-4, default: 3)
	-txindex               Maintain a full transaction index (default: 0)
	-loadblock=<file>      Imports blocks from external blk000??.dat file
	-reindex               Rebuild block chain index from current blk000??.dat files
	-par=<n>               Set the number of script verification threads (up to 16, 0 = auto, <0 = leave that many cores free, default: 0)

Block creation options:

	-blockminsize=<n>      Set minimum block size in bytes (default: 0)
	-blockmaxsize=<n>      Set maximum block size in bytes (default: 250000)
	-blockprioritysize=<n> Set maximum size of high-priority/low-fee transactions in bytes (default: 27000)
					       SSL options: (see the Bitcoin Wiki for SSL setup instructions)
	-rpcssl                                  Use OpenSSL (https) for JSON-RPC connections
	-rpcsslcertificatechainfile=<file.cert>  Server certificate file (default: server.cert)
	-rpcsslprivatekeyfile=<file.pem>         Server private key (default: server.pem)
	-rpcsslciphers=<ciphers>                 Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)
         
UI options:

	-lang=<lang>           Set language, for example "de_DE" (default: system locale)
	-min                   Start minimized
	-splash                Show splash screen on startup (default: 1)

Many of the boolean options can also be set to off by specifying them with a "no" prefix: e.g. -nodnseed.