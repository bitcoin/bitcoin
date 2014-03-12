Coding
====================

Please be consistent with the existing coding style.

Block style:

	bool Function(char* psz, int n)
	{
	    // Comment summarising what this section of code does
	    for (int i = 0; i < n; i++)
	    {
	        // When something fails, return early
	        if (!Something())
	            return false;
	        ...
	    }
	
	    // Success return is usually at the end
	    return true;
	}

- ANSI/Allman block style
- 4 space indenting, no tabs
- No extra spaces inside parenthesis; please don't do ( this )
- No space after function names, one space after if, for and while

Variable names begin with the type in lowercase, like nSomeVariable.
Please don't put the first word of the variable name in lowercase like
someVariable.

Common types:

	n       integer number: short, unsigned short, int, unsigned int, int64, uint64, sometimes char if used as a number
	d       double, float
	f       flag
	hash    uint256
	p       pointer or array, one p for each level of indirection
	psz     pointer to null terminated string
	str     string object
	v       vector or similar list objects
	map     map or multimap
	set     set or multiset
	bn      CBigNum

-------------------------
Locking/mutex usage notes

The code is multi-threaded, and uses mutexes and the
LOCK/TRY_LOCK macros to protect data structures.

Deadlocks due to inconsistent lock ordering (thread 1 locks cs_main
and then cs_wallet, while thread 2 locks them in the opposite order:
result, deadlock as each waits for the other to release its lock) are
a problem. Compile with -DDEBUG_LOCKORDER to get lock order
inconsistencies reported in the debug.log file.

Re-architecting the core code so there are better-defined interfaces
between the various components is a goal, with any necessary locking
done by the components (e.g. see the self-contained CKeyStore class
and its cs_KeyStore lock for example).

-------
Threads

- ThreadScriptCheck : Verifies block scripts.

- ThreadImport : Loads blocks from blk*.dat files or bootstrap.dat.

- StartNode : Starts other threads.

- ThreadGetMyExternalIP : Determines outside-the-firewall IP address, sends addr message to connected peers when it determines it.

- ThreadDNSAddressSeed : Loads addresses of peers from the DNS.

- ThreadMapPort : Universal plug-and-play startup/shutdown

- ThreadSocketHandler : Sends/Receives data from peers on port 8333.

- ThreadOpenAddedConnections : Opens network connections to added nodes.

- ThreadOpenConnections : Initiates new connections to peers.

- ThreadMessageHandler : Higher-level message handling (sending and receiving).

- DumpAddresses : Dumps IP addresses of nodes to peers.dat.

- ThreadFlushWalletDB : Close the wallet.dat file if it hasn't been used in 500ms.

- ThreadRPCServer : Remote procedure call handler, listens on port 8332 for connections and services them.

- BitcoinMiner : Generates bitcoins (if wallet is enabled).

- Shutdown : Does an orderly shutdown of everything.
