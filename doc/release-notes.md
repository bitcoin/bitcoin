(note: this is a temporary file, to be added-to by anybody, and deleted at
release time)

Fee changes

Fee-handling code has been rewritten, so that transaction fees paid are adjusted
based on the transactions that miners are including in blocks instead of
being arbitrarily hard-coded.

RPC changes

getrawmempool : now has an optional 'verbose' boolean flag, if true
reports information on each memory pool transaction.

New RPC methods

estimatefees : Returns estimate of priority or fee needed to get
transactions accepted into blocks.

Command-line changes

-limitfreerelay and -minrelaytxfee options are obsolete. 
They are replaced with dropping transactions that are unlikely
to be included in the next several blocks.

