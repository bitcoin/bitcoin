0.10.x Release notes
====================

This branch was released open source with v0.10.14 on Sept/25/2014. The
changelog reaches back further but remains valid as all commits are visible now.


0.10.16.7-16 Release notes
--------------------------

- Fixed queuing system timeout values (causing mixing issues)


0.10.16.7-15 Release notes
--------------------------

- Fixed memory issues
- Fixed remote masternode issues
- Fixed darksend definitions
- Fixed mixing issues
- Reduced collateral charges by 90%
- Changed ping time to 5m


0.10.16.6 Release notes
-----------------------

- saction for Darksend. These are special transactions that require a signature that only the masternodes can create.
- Darksend now has no fees to track what-so-ever, all that will ever be in Darksend transactions are Darksend denominations.
- Added queue gaming protection
- Clients remember which masternodes they've connected to in the past and won't use them against.
- Dsee/Dseep messages have been fixed so they only take newer signatures than the one they have
- 2 different kinds of client crashes have been fixed
- Split up main.cpp into core.cpp
- Split up darksend.cpp into masternode.cpp, activemasternode.cpp and instantx.cpp
- Added modular ProcessMessages for Darksend, Masternodes and InstantX
- Client can now join sessions with any other users

0.10.15.20/21 Release notes
---------------------------

- added bloom filters for dsee/dseep broadcasts, moved expensive dsee search, masternode vote caching


0.10.15.19 Release notes
------------------------

- Added sanity check and debugging


0.10.15.18 Release notes
------------------------

- Fixed masternode payment check for out-of-order blocks


0.10.15.17 Release notes
------------------------

- fixed GetBlockPayee


0.10.15.16 Release notes
------------------------

- masternode only take newest dsee now


0.10.15.15 Release notes
------------------------

- fixed fragmentation issue, allow masternodes to update pubkey2


0.10.15.14 Release notes
------------------------

- fixed dsee duplication issue and added better cleanup for
  inactive masternodes


0.10.15.13 Release notes
------------------------

- Onyx Release!


0.10.15.12 Release notes
------------------------

- Added some debugging log output to track down why some users are still
  getting charged collateral. If anyone gets hit with a collateral fee,
  please send me the debu.log so I can check it out.
- Includes a possible fix to the collateral issue
- Protocol bump, all users must update!


0.10.15.10 Release notes
------------------------

- Fixed a couple more collateral charge issues, plus an issue causing
  "incompatible denominations" when it should have worked. Mixing
  should be faster now.


0.10.15.9 Release notes
------------------------

- Fixed a race condition causing collateral changes in rare cases.


0.10.15.8 Release notes
------------------------

- Fixed all sorts of issues with collateral and Darksend.
  I tested five rounds and it worked flawlessly.


0.10.15.7 Release notes
------------------------

- Fixed the stuck blockchain (due to masternode payments)
- Improves collateral creation for those getting "invalid collateral"
- Minimum confirmation requirements before masternode payment.
  Must have more confirmations then there are masternodes.


0.10.15.6 Release notes
------------------------

- This updates the min protocol version to kick off older clients
- Improves collateral charging
- Changes the masternode payments keys, you'll only recieve masternode
  payments and be able to mine on 15.6
- If anyone has that warning message, start the client
  with --disablesafemode --reindex


0.10.15.5 Release notes
------------------------

- I've made some huge progress with enforcement. Each masternode will
  have exactly the same distribution of payments after this.


0.10.15.4 Release notes
------------------------

- Fixed some issues with masternode payments syncing


0.10.15.3 Release notes
------------------------

- Incremented protocol / protocol version requirements due
  to the testnet-hardfork


0.10.15.2 Release notes
------------------------

- Fixed some issues with the way the masternode payment list
  was being handled
- Masternode confirmation error now displays 200 required instead of 6
- Changed the pinging settings around
- Block reward reduction is set to have it's first 7% reduction in a couple
  days on testnet to test that code


0.10.15.1 Release notes
------------------------

- Darksend tested with 2.5 DRK successfully and I've set that as the minimum
  amount that's compatible
- Fixed a locking bug on inputs
- Fixed a collateral calculation bug


0.10.14.1 Release notes
------------------------

- security update: This update simply verifies that the signature was
  not forged and that the masternodes entering the list are authentic.


0.10.14.0 Release notes
------------------------

- community release


0.10.13.14 Release notes
------------------------

- RC5 released!


0.10.13.13 Release notes
------------------------

- Bounds checking in a few places where it was lacking
- Output list in transactions lacked random seeding
- masternode constants for communication are much easier to read now


0.10.13.12 Release notes
------------------------

- disconnect on rejecting enforced block


0.10.13.11 Release notes
------------------------

- Improved handling of enforcement for bad masternode lists
- Removed old masternode override code


0.10.13.10 Release notes
------------------------

- Changed splitting strategy to deal with some edge cases (endless splitting for a few users)
- Updated stable proto version


0.10.13.9 Release notes
-----------------------

- Testnet merges use two, while mainnet merges will use 3 participants
- Fixed the endless splitting issue causes by splitting 1000DRK and not making a DS compatible input


0.10.13.8 Release notes
-----------------------

- Debugged progress bar
- New terms of use window
- Darksend UI is disabled for masternodes now and titlebar says "[masternode]"
- Improvement for dealing with splitting large inputs
- Protocol version bump to kick old masternodes off


0.10.13.7 Release notes
-----------------------

- Redid the way the progress calculation works (should be smoother now and accurate)
- Updated stable to prepare for enforcement testing


0.10.13.6 Release notes
-----------------------

- Added toggle off/on button to overview screen
- Updated wording for darksend messages
- Removed disable darksend checkbox from config screen
- Added "-enabledarksend" cli option
- Toggle button shows the basic configuration when started for
  the first time now
- Added tooltips for config screen
- Changed DS participants to three
- Bump minimum protocol to RC4
- Added a spork for enforcing masternode payments (this will ensure misconfigured
  pools break when we enable the spork)


0.10.13.5 Release notes
-----------------------

- This should fix the endless splitting bug
- Overview updates when you update settings and click OK now


0.10.13.4 Release notes
-----------------------

- Fixed issue with denominating small amounts of DRK in large wallet
  (http://jira.darkcoin.qa/browse/DRK-46)
- Made splitting up initial inputs much more efficient. Now when splitting up,
  it will use powers of two from 4096 DRK in reverse to get the best possible
  mix of inputs for the next phase without any bloat to the blockchain or to
  the users wallet.
- Fixed "transaction too large" due to the initial splitting function
- Stopeed collateral/fee creation when it should have been doing a full
  split instead
- Sometimes the client would denominate less than the intended amount, then do
  small denominations to make up the difference. This slowed down the
  transactions and created extra transactions that weren't needed.
- Darksend should anonymize very close to the intended amount now
- Added Amount/Rounds to overview screen so you can see current settings
- Overview darksend cache is cleared on settings change (will instantly update)
- Fixed issue with completed amount jumping around
  (http://jira.darkcoin.qa/browse/DRK-46)
- Made messages less threatening (http://jira.darkcoin.qa/browse/DRK-60)

PS : Please move testing funds to a new wallet. This version has massive
optimizations for the way inputs are stored and split up. This will make
everything much more efficient.


0.10.13.3 Release notes
-----------------------

- This update deals with freezing, slow wallets, slow load times and the
  "not compatible" error. I debugged one of the slow wallets and found it had
  38,000 keys in the keypool, then after more investigation I found the passive
  Darksend process has been reserve keys for every attempt! To rectify this
  I've modified the queuing system, so users wait in a masternode queue without
  actually sending the transactions until the queue is full.

Please move any testing funds to a new wallet to test the new version.


0.10.13.2 Release notes
-----------------------

- Easy to read darksend progress bar (Mouse over for very detailed info)
- Fixed a collateral bug
- Moved around the overview screen, changed some of the text
- Removed lots of debug messages (they show up only with debug=1 now)


0.10.13.1 Release notes
-----------------------

- Fixed a bug where the client gives the error "No funds detected in need of
  denominating (2)" when it should have split


0.10.12.27 Release notes
------------------------

- removed inputs < 1 COIN from DS


0.10.12.26 Release notes
------------------------

- minor bugfix


0.10.12.25 Release notes
------------------------

RC4 released!


0.10.12.24 Release notes
------------------------

- Reverted the pairing fix, seems the network didn't improve at all.
- Inc protocol version


0.10.12.23 Release notes
------------------------

- Fixed an issue with pairing, hopefully it should be faster now
- The client can now recongize it's out of fee inputs and make some more.
- erasetransaction help (thanks BelStar)
- inc protocol to kick off old versions


0.10.12.22 Release notes
------------------------

All fees will use 0.001 sized inputs (they have no change so you can't follow
them), all transactions now should look like this one:

http://test.explorer.darkcoin.fr/tx/ce0ea2bdf630233955d459489b6f764e0d0bbe9e8a62531dd2a14b455626b59c

- Client now creates fee sizes inputs for use in darksend denomination phases
  to improve anonymity
- new rpc command erasetransaction. Some users have reported non-confirming
  transactions due to opening their wallet at multiple locations. This can and
  will create double spent transactions that will not confirm. erasetransaction
  is for removing them.
- SplitUpMoney can only execute every 10 blocks now.
- removed matching masternode debugging messages, that's not really an error
- Client now prioritises sending denominated funds back into Darksend. This will
  improve anonymity and help to respect the "anonymize darkcoin" amount in the
  configuration.
- fixed a bug where masternodes send failed transactions
- changed max to 100k in configuration
- added a warning message to startup (delete ~/.darkcoin/.agree_to_tou to see it)
- found a bug causing inputs to get locked forever.
- Darksend now checks diskspace before sending anything to a masternode.
- incrementing protocol version to knock all old clients off


0.10.12.21 Release notes
------------------------

- Majorly improved the way darksend participants are paired together. It should
  be super fast now.


0.10.12.20 Release notes
------------------------

- Disabled collateral charging for now. We'll work on this after RC4 is
  released and update the masternode network after it's working properly. It's
  not incredibly important at this stage (while we're closed source), so
  I don't want it holding up the release. Plus it's really
  the only issue we're experiencing
- Merged rebroad's (https://github.com/rebroad) changes to bring debugging
  output more in line with the bitcoin project. Output is now much cleaner and
  can be split by category.
- Removed some debug messages
- Merged mac/windows build icons
- Fixed windows "Apply" configuration bug
- Darksend now shows address instead of "n/a"
- Incremented protocol version to kick off old versions that charge fees. Fees
  should be completely gone now.
- Found a bug that was causing "not compatible" errors too often. This should
  speed  up pairing.


0.10.12.19 Release notes
------------------------

- Added GUI configuration for Darksend Rounds, Enable Darksend and Anonymize
  amount of DRK
- Removed 5000DRK hard limit
- Fixed another cause of getting hit by collateral
- Send dialog now shows selected balance (Anon, non-anon and Total)


0.10.12.18 Release notes
------------------------

This version uses the new queuing system to seek out compatible transactions
(where the same denominations are used). It's also enforcing these limitations
now, so it might be a bit slower.

All transactions after this should look like this one:

http://test.explorer.darkcoin.fr/tx/6de2c5204abdea451da930f61bae0f954eef13188a3a37a572a24c9d92057d5d


0.10.12.17 Release notes
------------------------

- I've switched up the way the masternode network works.
    1.) Users now will join a random masternode (1 of the entire list, just
        completely randomly)
    2.) Upon joining if it's the first user, the masternode will propagate
        a message stating it's taking participants for a merge
    3.) Another user will check that queue, if it's got a recent node, it will
        try that node first, otherwise it will go to 1.)

- Darksend limited to 5000DRK per wallet.dat. Client will warn about this the
  first time it's opened, then disable darksend from then on.
- Fixed some bugs with connecting to the correct masternodes
- Send was sending way too many coins for all modes, (I sent 100DRK anon and it
  sent 2000DRK, then sent me change for the rest causing a whole reprocess of
  everything in the wallet)
- Client now updates Anonymized balance when you send money out
- Fixed coin locking issues


0.10.12.16 Release notes
------------------------

- bugfixes


0.10.12.15 Release notes
------------------------

- Added session IDs for masternode communication. Clients were getting
  confused when they got messages about other sessions (sometimes happened when
  they all jumped on the same masternode at once)
- Added a pre-session state where the client will query a random masternode
  and ask if they can perform a merge on N darkcoin without giving any other
  information. If that amount is compatible without losing anonymity, the client
  will then add it's entry for merging
- Added code to randomly use the top 20 masternodes, this can dynamically be
  increased as more transaction traffic starts to happen (although it's not
  implemented but it could be done later)
- After successful transactions clients will now automatically attempt another
  session on a random masternode, then repeat until they get any kind of error
  or run out of funds that need to be processed.
- Fixed a change address reuse issue
- Fixed an issue with the compatible join algorithm (Masternodes will only join
  the same denominations, this wasn't always the case before)
- Inc protocol to kick old users odd again


0.10.12.14 Release notes
------------------------

- Fixed an issue where clients weren't connected to the correct masternode
- Fixed masternode relay issues
- Anonymous Balance now calculates correctly
- Inc protocol to kick old users odd again


0.10.12.13 Release notes
------------------------

- This version automatically resets the masternode state machine after a short
  period of inactivity.
- Updated protocol version to kick old masternodes off


0.10.12.12 Release notes
------------------------

- Fixed change calc for Denominations in GUI
- Flare found a logging error for dseep, fixed
- Collateral now includes a fee (sometimes they took forever to get into a block)
- Found race condition with new blocks and clearing darksend entries that was
  causing some collateral fees
- Found a communication mix up where clients would see messages from the wrong
  masternode and think it was theirs, also causing collateral fees
- Added "Anonymized Balance" to overview
- Added "anonymized_balance" to getinfo
- Changed dropbox box on Send Dialog to be clearer
- Added text to the confirmation screen with what funds will be sent
- incremented protocol version to force masternode updates


0.10.12.11 Release notes
------------------------

- Darksend Denominate Outputs are now in a random order:

http://test.explorer.darkcoin.fr/tx/072ca56cbf705b87749513a2d2ee02080d506adcf8fe178f6dc2967f0711788e
http://test.explorer.darkcoin.fr/tx/32daa8ca46462e7e99f3532251d68a8c3835a080c937bd83b11db74e47b770ff

- Darksend now uses 3 participants instead of two.
- SplitUpMoney can now make collateral inputs when needed
- Transactions now shows darksend transaction types for easier understanding
  of what's going on:
- Fixed a couple more cases where collateral was charged when it shouldn't
  have happened (let me know if it happens after this version)
- Fixed the money destruction bug, it was caused by "darksend denominate 8000".
  I missed a reference and the client passed an empty address to
  SendMoneyToDestination. rcp darksend source: http://pastebin.com/r14piKuq
- Unlocking/Locking wallet fixes (was spamming the logs)
- Unencrypted wallet fixes (was trying to lock every 10 seconds)
- Flare found and fixed an issue with DGW3 for win32
- Added Darksend detection to the UI
- fixed senttoaddress, it will use all inputs when darksend is disabled now.
  Otherwise it will ONLY use non-denom.
- "darksend addr amount" now returns the hash of the transaction it creates


0.10.12.10 Release notes
------------------------

Another huge update to the RC client, most of these are stability fixes and
anonymity improvements:

- Removed "darksend denominate", darksend now will figure out the most it can
  denominate. Use "darksend auto" instead.
- Fixed "Unknown State" display error
- Fixed 0.0025 collateral issues caused by issues in the state machine, you
  should only be charged this amount now if you shutdown your client during
  the Darksend process.
- Client will only submit 1 transaction into the pool fixing possible
  anonymity issues
- Masternodes will only merge compatible transactions using the same
  denominations. For example (500,500,100) would be able to merge
  with (500,100), (10,1) with (10,1,1), but not (500,1) with (10,1).
  This improves the anonmity by not allowing someone to follow transactions by
  the missing denominations.
- Transactions use unique change addresses for every output of each round.
- QT GUI will now ask to unlock the wallet when it detects Darksend wants to do
  something and lock it when it's done again.
- Darksend is turned off by default in the daemon now. In most cases daemons
  won't want to run with anonymity (pools, exchanges, etc), if a user does they
  can override the default setting with -enabledaemondarksend=true
- Fees per round of Darksend are 0.001DRK or $0.00538 at current prices. This
  means to anonymize 1000DRK with 3 rounds (an average use case) it would cost
  a user 1.5 cents.
- Protocol version is updated to kick old clients off testnet


0.10.12.9 Release notes
-----------------------

DS+ seems to be pretty stable now :-)

- SplitUpMoney now calculates the balance correctly
- Denominations are now 1 satoshi higher (denominated inputs will have to be
  regenerated as the client will not recognize the old ones)
- SplitUpMoney does a better job of splitting up really large wallets now
- Fixed crashing issues
- Added possible fix for masternode list syncing

- RPC calls are changed a bit:

    darksend denominate 100 - Will denominate 100DRK
    darksend auto - Will run AutoDenominate
    darksend Xaddr 100 - Will send 100 denominated DRK to Xaddr
    sendtoaddress Xaddr 100 - Will send 100 non-denominated DRK to Xaddr


0.10.12.8 Release notes
-----------------------

- Fixed a few issues with input selection causing the
  "Insufficent Funds 2" error
- Masternodes now reset themselves when they give "entries is full".
  Not sure what's causing it but a client will just try again
- Improved the split up function
- Fixed issues with AutoDenom in wallets larger than a few hundred
- Fixed a case for collateral charges where the client gave up
  when it shouldn't have
- Input selection will now only select denominated, non-denominated or
  all inputs. This caused ds+ inputs to get interrupted by the splitting
  mechanism sometimes.
- Added new GUI element for selecting which inputs you want to send
- Fixed darksend+ rounds function, it was returning bad data in some cases
  causing darksend inputs to never stop being sent through darksend.
- Fixed "Send" dialog to be able to use the different kinds of inputs available.
  Sending anonymous transactions should now work properly.
- Fixed some coin select issues
- Collateral selection issues
- SplitUpMoney was sending denominated inputs and destroying the anonymity

DoAutoDenominate should work in nearly all cases now.
However, there are some known issues:

- Random collateral charges (still will happen, but it's more uncommon.)
- Password protected wallets


Testing commands, you can start multiple wallets up and all denominate
on the same masternode for testing purposes:
/darkcoin-qt -datadir=/home/user/.darkcoin -listen=0 -darksendrounds=8 -usemasternode="192.168.56.102:19999"
/darkcoin-qt -datadir=/home/user/.darkcoin2 -listen=0 -darksendrounds=8 -usemasternode="192.168.56.102:19999"

and even disable darksend auto-denom if wanted:
/darkcoin-qt -datadir=/home/user/.darkcoin -listen=0 -darksendrounds=8 -usemasternode="192.168.56.102:19999" -disabledarksend=1


0.10.12.7 Release notes
-----------------------

- Added a smart input splitting method. Place 1000+DRK into a brand new wallet
  and it will be split into many inputs compatible with Darksend
- DoAutodenomination now tries the correct balance (it was getting stuck on
  the wrong inputs)
- "entries is full" fix for at least one of the causes
- Changed merging parties to two for easier debugging.
- Fixed mod again (missed the one for the actual command you guys are using,
  I was overriding the default there)


0.10.12.6 Release notes
-----------------------

- Fixed AutoDenominate. It seems to work pretty well now.
- Inputs that are large will be broken up automatically for denomination
- Masternodes should change every block now (missed a mod=10 last time)
- Mixing requires 5 clients to merge now, should improve anonymity.
- Mixing rounds are limited to 1000DRK, per block


0.10.12.5 Release notes
-----------------------

- Masternodes should change every block now
- DoAutomaticDenomination should happen every block now
- DarkSendRounds had a bug that I fixed, should calculate correctly now


0.10.12.4 Release notes
-----------------------

This is a pretty large update to the RC client.

- New column "Darksend Rounds" in coincontrol to show how secure a given input is
- Fixed a few issues causing darksend to fail. We should see many more darksends
  occuring now if it's fixed.
- Redid denominations to 1, 10, 100, and 500. Maybe this is too simple, but it
  seems effective, all change from transactions will de denominated automatically
  again through darksend for the next transactions. We'll see how it works.
- usemasternode option, will override active masternode (only in RC, just for testing)

0.10.12.3 Release notes
-----------------------

- min merged transactions

0.10.12.2 Release notes
-----------------------

- Fixed payout issues (masternode consessus was paying out to vout(0) by default)
- Improved DarksendInput add entry verification. Masternodes will now reject
  transactions that look like fees are too low, too high, have spent inputs, etc.
- Incremented protocol version to kick off clients with vout(0) payment bug
- DoAutomaticDenominations 100DRK limit changed to 500DRK (we should see a bunch
  of denominations happen now)


0.10.12.1 Release notes
-----------------------

- Fixed a signing bug with the masternode voting system causing a bunch of issues
- Updated unit tests
- Incremented protocol version to kick off clients with signing bug


0.10.11.6 Release notes
-----------------------

- resolves issue with wallet not syncing by adding the capability to retrieve nodes through dnsseed
   (see https://github.com/darkcoinproject/darkcoin/pull/21)
- Linux 32 and Mac OS X are now officially supported platforms
   (see https://github.com/darkcoinproject/darkcoin/pull/17)
- improved overall unit test code coverage
   (see https://github.com/darkcoinproject/darkcoin/pull/13 and https://github.com/darkcoinproject/darkcoin/pull/15)
- minor documentation updates
   (https://github.com/darkcoinproject/darkcoin/pull/18)
- improved distribution packaging (win: zip, linux: tar.gz, osx: dmg)
