(note: this is a temporary file, to be added-to by anybody, and deleted at
release time)

Fee Policy changes
------------------

The default fee for low-priority transactions is lowered from 0.0005 BTC 
(for each 1,000 bytes in the transaction; an average transaction is
about 500 bytes) to 0.0001 BTC.

Payments (transaction outputs) of 0.543 times the minimum relay fee
(0.00005430 BTC) are now considered 'non-standard', because storing them
costs the network more than they are worth and spending them will usually
cost their owner more in transaction fees than they are worth.

Non-standard transactions are not relayed across the network, are not included
in blocks by most miners, and will not show up in your wallet until they are
included in a block.

The default fee policy can be overridden using the -mintxfee and -minrelaytxfee
command-line options, but note that we intend to replace the hard-coded fees
with code that automatically calculates and suggests appropriate fees in the
0.9 release and note that if you set a fee policy significantly different from
the rest of the network your transactions may never confirm.

Bitcoin-Qt changes
------------------

- New icon and splash screen
- Improve reporting of synchronization process
- Remove hardcoded fee recommendations
- Improve metadata of executable on MacOSX and Windows
- Move export button to individual tabs instead of toolbar
- Add "send coins" command to context menu in address book
- Add "copy txid" command to copy transaction IDs from transaction overview
- Save & restore window size and position when showing & hiding window
- New translations: Arabic (ar), Bosnian (bs), Catalan (ca), Welsh (cy), Esperanto (eo), Interlingua (la), Latvian (lv) and many improvements to current translations

MacOSX:

- OSX support for click-to-pay (bitcoin:) links
- Fix GUI disappearing problem on MacOSX (issue #1522)

Linux/Unix:

- Copy addresses to middle-mouse-button clipboard


Command-line options
--------------------

* `-walletnotify` will call a command on receiving transactions that affect the wallet.
* `-alertnotify` will call a command on receiving an alert from the network.
* `-par` now takes a negative number, to leave a certain amount of cores free.

JSON-RPC API changes
--------------------

* `listunspent` now lists account and address infromation.
* `getinfo` now also returns the time adjustment estimated from your peers.
* `getpeerinfo` now returns bytessent, bytesrecv and syncnode.
* `gettxoutsetinfo` returns statistics about the unspent transaction output database.
* `gettxout` returns information about a specific unspent transaction output.


Networking changes
------------------

* Significant changes to the networking code, reducing latency and memory consumption.
* Avoid initial block download stalling.
* Remove IRC seeding support.
* Performance tweaks.
* Added testnet DNS seeds.

Wallet compatibility/rescuing
-----------------------------

* Cases where wallets cannot be opened in another version/installation should be reduced.
* `-salvagewallet` now works for encrypted wallets.

