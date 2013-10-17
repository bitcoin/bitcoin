### Wallet Tools ###
These are two simple python scripts which send the appropriate RPC commands to unlock a wallet and change a wallet password. **They are intended to prevent users from having to enter their password as a command-line argument which could then be stored in the console buffer/history in plaintext.**

Both tools rely on bitcoin/bitcoind running with `server=1` and an `rpcuser` and `rpcpassword` set in `bitcoin.conf`. They can be easily modified for non-standard ports. [walletunlock.py](/contrib/wallettools/walletunlock.py) unlocks the wallet for 60 seconds by default, changeable in code, and both modules rely upon python-json-rpc.
