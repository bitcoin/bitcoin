Binaries for Crowncoin version 0.3.21 are available at:
  https://sourceforge.net/projects/crowncoin/files/Crowncoin/crowncoin-0.3.21/

Changes and new features from the 0.3.20 release include:

* Universal Plug and Play support.  Enable automatic opening of a port for incoming connections by running crowncoin or crowncoind with the - -upnp=1 command line switch or using the Options dialog box.

* Support for full-precision crowncoin amounts.  You can now send, and crowncoin will display, crowncoin amounts smaller than 0.01.  However, sending fewer than 0.01 crowncoins still requires a 0.01 crowncoin fee (so you can send 1.0001 crowncoins without a fee, but you will be asked to pay a fee if you try to send 0.0001).

* A new method of finding crowncoin nodes to connect with, via DNS A records. Use the -dnsseed option to enable.

For developers, changes to crowncoin's remote-procedure-call API:

* New rpc command "sendmany" to send crowncoins to more than one address in a single transaction.

* Several bug fixes, including a serious intermittent bug that would sometimes cause crowncoind to stop accepting rpc requests. 

* -logtimestamps option, to add a timestamp to each line in debug.log.

* Immature blocks (newly generated, under 120 confirmations) are now shown in listtransactions.
