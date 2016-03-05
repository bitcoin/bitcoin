FreeTrade (30):
      bitcoin.conf -> hodlcoin.conf
      bitcoin->hodlcoin
      remove test that was preventing Ubuntu compile
      Improved randomness for nonces
      Fix for address mining
      Fix reporting of immature balance
      Fix crash on miner stop
      Output hashes per second measurement to debug log
      Improved accuracy on hash/sec measure
      minor edits, unused variable, extra accuracy for hash calculator
      New seeds, updated warning message.
      Added churn option and dns for nodes
      Fix to show term deposits as immature balances again
      First checkpoint
      Fix for set generate false/true - no more crashes.
      Miner can use more than 1GB memory     Block size limited to 1MB protection against spam attacks     Additional Seed
      Coin Control fixes to show interest
      Changed unix directory from .bitcoin -> .hodlcoin
      Show input and interest amount in coin control
      GUI for making Term deposits
      Mining menu and status icon.
      HOdlings panel
      Update immature balance to show maturity value rather than accrued value of Hodlings.
      Narrowed columns to fit HOdlings table in default interface size.
      Change GB required to MB required
      Updated alert key. Private key held by FreeTrade
      Re-enabled isinitialblockdownload and adjusted copyright message
      Added famous Nutoshi Sakamoto quotes to splash page
      Message box for failed hodl transactions. Solution not ideal, but functional.
      Testnet genesis block. Testnet on port 8989

Fuzzbawls (34):
      HOdlcoin binary rebranding
      [Qt] Fix coin control column mismatch.
      [Qt] Fix total amnt shown in coin control UI tree view.
      More rebranding from "Bitcoin" to "HOdlcoin"     - Also updated the Mac icon to be the same as windows icon     - Also fixed the Mac notification registry
      [Core] Re-add gethashespersec RPC support for solo mining     - getmininginfo now returns the current HPS value when setgenerate is true.
      [Qt] Fix Mining icon and tooltip
      [Gitian] Update Gitian descriptors for hodlcoin.
      Rebrand client version for initial 1.0.0.0 release.
      More rebranding from "Bitcoin" to "HOdlcoin"
      [Build] Correct some mac specific build issues
      [Travis] Disable post-build RPC checks for now.
      [Depends] Use curl for fetching on Linux     Currently Travis's wget fails fetching qrencode:
      [Travis] Fix typo in OSX SDK fetching
      [Travis] Change the "No Wallet" compile to full featured 64-bit Linux
      [Build] a "Bitcoin" reference slipped through the cracks for OS X builds.
      [Core] Fix gethashespersec RPC calculation return
      [Travis] Update OSX SDK URL
      [Travis] Add slack notifications
      [Core] Remove orphaned AES capability check     The AES CPU capability check is unused and     causes issues when building on ARM processors.
      [Travis] Re-enable ARM test
      New Logo/Icon graphic rebranding
      Fixup the new icon a bit more
      Apply a dual copyright where applicable to retain the BTC copyright.
      [Qt] Adjust the splashscreen rendering to work with the new Icon
      [Qt] typo in splashscreen.cpp
      Update README.md
      Update README.md
      Begone you stable double-quote!
      [Qt] Normalize Deposits Tab     Added Coin Control     Added Fee Chooser     Added Balance Total
      [Qt] Remove APR column from HOdlings Table
      [Core] User supplied miningaddress fixes     In-Wallet miner will now validate the user supplied mining address
      Rebranding for URI handling
      [Gitian] Update descriptors for HOdlcoin 1.0
      HOdlcoin Version 1.0.0 Release

Stoner19 (1):
      line 101 changed to LogPrintf

Vetro7 (3):
      Updated Readme.md
      Replaced with windows guide
      Updated Readme.md

pallas1 (1):
      Fix possible silent crash.

