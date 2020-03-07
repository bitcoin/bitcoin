### SpendFrom ###

Use the raw transactions API to send coins received on a particular
address (or addresses). This is similar to the coin-control functionality
of the GUI wallet and is useful for consolidating rewards.

### Usage: ###
Depends on [python-bitcoinrpc](http://www.github.com/jgarzik/python-bitcoinrpc/).

	spendfrom.py --from=FROMADDRESS1[,FROMADDRESS2] --to=TOADDRESS \
                     --amount=amount [--fee=fee] \
                     [--datadir=/path/to/.crown] [--config=config-file-name] \
                     [--upto] [--dry_run] [--testnet] [-v [-v [-v]]]

With no arguments, outputs a list of amounts associated with addresses.

With arguments, sends coins received by the `FROMADDRESS` addresses to the 
`TOADDRESS`.

### Notes ###

- You may explicitly specify how much fee to pay (a fee more than 1% of the 
amount will fail, though, to prevent Crown-losing accidents). Spendfrom may 
fail if it thinks the transaction would never be confirmed (if the amount 
being sent is too small, or if the transaction is too many bytes for the fee). 
If you specify a really small fee the transaction may sit in the mempool for 
a long time before it is added to a block. If you're consolidating rewards a 
fee between 0.01 and 0.1 CRW is usually sufficient for the transaction to be 
added to the next block. The default fee is 0.025 CRW.

- If a change output needs to be created, the change will be sent to the last
`FROMADDRESS` (if you specify just one `FROMADDRESS`, change will go back to 
it).

- If `--to=new` is specified a new receiving address is created in the local
wallet. 

- If `--datadir` is not specified, the default datadir is used.

- If `--config` is not specified, the default crown.conf is used.

- The `--upto` option will not complain if there are insufficient funds for
the amount requested, and just send as much as is available.

- The `--dry_run` option will just create and sign the transaction and print
the transaction data (as hexadecimal), instead of broadcasting it.

- The `--testnet` option is self-explanatory.

- The `-v` option can be used (multiple times) to increase verbosity and 
produce informational/debugging messages.

- If the transaction is created and broadcast successfully, a transaction id
is printed.

- UTXO selection policies are not yet implemented. 

- This is becoming a tool for end-users with improved and friendlier 
error-handling.
