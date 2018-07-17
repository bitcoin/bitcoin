Bitcoin-Qt version 0.8.2 is now available from:
  http://sourceforge.net/projects/bitcoin/files/Bitcoin/bitcoin-0.8.2/

This is a maintenance release that fixes many bugs and includes
a few small new features.

Please report bugs using the issue tracker at github:
  https://github.com/bitcoin/bitcoin/issues


How to Upgrade

If you are running an older version, shut it down. Wait
until it has completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
/Applications/Bitcoin-Qt (on Mac) or bitcoind/bitcoin-qt (on Linux).

If you are upgrading from version 0.7.2 or earlier, the first time you
run 0.8.2 your blockchain files will be re-indexed, which will take
anywhere from 30 minutes to several hours, depending on the speed of
your machine.

0.8.2 Release notes

Fee Policy changes

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

* New icon and splash screen
* Improve reporting of synchronization process
* Remove hardcoded fee recommendations
* Improve metadata of executable on MacOSX and Windows
* Move export button to individual tabs instead of toolbar
* Add "send coins" command to context menu in address book
* Add "copy txid" command to copy transaction IDs from transaction overview
* Save & restore window size and position when showing & hiding window
* New translations: Arabic (ar), Bosnian (bs), Catalan (ca), Welsh (cy),
  Esperanto (eo), Interlingua (la), Latvian (lv) and many improvements
  to current translations

MacOSX:
* OSX support for click-to-pay (bitcoin:) links
* Fix GUI disappearing problem on MacOSX (issue #1522)

Linux/Unix:
* Copy addresses to middle-mouse-button clipboard


Command-line options

* -walletnotify will call a command on receiving transactions that affect the wallet.
* -alertnotify will call a command on receiving an alert from the network.
* -par now takes a negative number, to leave a certain amount of cores free.

JSON-RPC API changes

* fixed a getblocktemplate bug that caused excessive CPU creating blocks.
* listunspent now lists account and address information.
* getinfo now also returns the time adjustment estimated from your peers.
* getpeerinfo now returns bytessent, bytesrecv and syncnode.
* gettxoutsetinfo returns statistics about the unspent transaction output database.
* gettxout returns information about a specific unspent transaction output.


Networking changes

* Significant changes to the networking code, reducing latency and memory consumption.
* Avoid initial block download stalling.
* Remove IRC seeding support.
* Performance tweaks.
* Added testnet DNS seeds.

Wallet compatibility/rescuing

* Cases where wallets cannot be opened in another version/installation should be reduced.
* -salvagewallet now works for encrypted wallets.


Known Bugs

* Entering the 'getblocktemplate' or 'getwork' RPC commands into the Bitcoin-Qt debug
console will cause Bitcoin-Qt to crash. Run Bitcoin-Qt with the -server command-line
option to workaround.

Thanks to everybody who contributed to the 0.8.2 release!

APerson241
Andrew Poelstra
Calvin Owens
Chuck LeDuc Díaz
Colin Dean
David Griffith
David Serrano
Eric Lombrozo
Gavin Andresen
Gregory Maxwell
Jeff Garzik
Jonas Schnelli
Larry Gilbert
Luke Dashjr
Matt Corallo
Michael Ford
Mike Hearn
Patrick Brown
Peter Todd
Philip Kaufmann
Pieter Wuille
Richard Schwab
Roman Mindalev
Scott Howard
Tariq Bashir
Warren Togami
Wladimir J. van der Laan
freewil
gladoscc
kjj2
mb300sd
super3
