Running Bitcoind
---------------------
Bitcoind is a headless daemon, and also bundles a testing tool for the same daemon. It provides a JSON-RPC interface, allowing it to be controlled locally or remotely which makes it useful for integration with other software or in larger payment systems. 

To use locally, first start the program in daemon mode:

	bitcoind -daemon

Then you can use the same program to execute API commands, e.g.:

	bitcoind getinfo
	bitcoind listtransactions

To stop the bitcoin daemon, execute:

	bitcoind stop

Bitcoind API
---------------------

Give Bitcoin (or bitcoind) the `-?` or `–-help` argument and it will print out a list of the most commonly used command-line arguments and then exit. Below is a list of the more commonly used commands:

	-?                     This help message
	-conf=<file>           Specify configuration file (default: bitcoin.conf)
	-gen                   Generate coins (default: 0)
	-datadir=<dir>         Specify data directory
	-timeout=<n>           Specify connection timeout in milliseconds (default: 5000)
	-dns                   Allow DNS lookups for -addnode, -seednode and -connect
	-port=<port>           Listen for connections on <port> (default: 8333 or testnet: 18333)
	-maxconnections=<n>    Maintain at most <n> connections to peers (default: 125)
	-addnode=<ip>          Add a node to connect to and attempt to keep the connection open
	-connect=<ip>          Connect only to the specified node(s)
	-seednode=<ip>         Connect to a node to retrieve peer addresses, and disconnect
	-externalip=<ip>       Specify your own public address
	-discover              Discover own IP address (default: 1 when listening and no -externalip)
	-listen                Accept connections from outside (default: 1 if no -proxy or -connect)
	-bind=<addr>           Bind to given address and always listen on it. Use [host]:port notation for IPv6
	-upnp                  Use UPnP to map the listening port (default: 1 when listening)
	-paytxfee=<amt>        Fee per KB to add to transactions you send
	-server                Accept command line and JSON-RPC commands
	-testnet               Use the test network
	-rpcuser=<user>        Username for JSON-RPC connections
	-rpcpassword=<pw>      Password for JSON-RPC connections
	-rpcport=<port>        Listen for JSON-RPC connections on <port> (default: 8332 or testnet: 18332)
	-rpcallowip=<ip>       Allow JSON-RPC connections from specified IP address
	-rescan                Rescan the block chain for missing wallet transactions
	-reindex               Rebuild block chain index from current blk000??.dat files

You may find a full list of command line switches by running:

	bitcoind --help >> switches

or RPC commands by running:

	bitcoind help >> commands

Additional information on the Bitcoind API is available on the [Bitcoin Wiki](https://en.bitcoin.it/wiki/Main_Page) under [Running Bitcoind](https://en.bitcoin.it/wiki/Running_Bitcoin).