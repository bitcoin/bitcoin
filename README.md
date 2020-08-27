Bitcoin Researcher
=====================================
This is a fork from the official Bitcoin Core main branch, which includes many additional features for testing the protocol and functionality of Bitcoin, as well as real-time logging of the node.

Instructions
----------------
For first time users, inside a Debian version of linux, run `./first_compile.sh` from the terminal to install all the necessary prerequisites, compile, and run Bitcoin.

If modifications are made to the code, run `./compile.sh` to only compile the code and run it, without the additional prerequisites/configurations from first_compile.sh.

If no modifications are made, run `./run.sh` to start up Bitcoin, with no additional overhead from compile.sh or first_compile.sh. To start in GUI mode instead of terminal mode, run `./run.sh gui`

By default, Bitcoin will store its blockchain in ~/.bitcoin. If this is the case, then run.sh will by default start Bitcoin in pruned mode, where it will only keep the last 550 blocks (a few gigabytes), to avoid the cost of a full blockchain (several hundred gigabytes).

If run.sh is successful, a window will show up labeled "Custom Bitcoin Console", which is a python script (bitcoin_console.py) that communicates with Bitcoin. Within this window, type `help` to list all the available commands.

By default, the newly added "researcher" logging category is activated, which prints out all the messages that are received by Bitcoin, along with the address of the sender, how long it took to process, and the number of bytes. This can be disabled in the console by entering `log researcher` to toggle the "researcher" category, or simply `log` to toggle between all logging categories, and disable them.

Modifications
----------------

A new category of commands exist under the label "Z Researcher". A few of them are as follows:
* DoS "duration" "times/seconds/clocks" "msg" ( "args" )
* blocktimeoffset
* bucketadd "address" "port"
* bucketclear
* bucketinfo
* bucketlist "new/tried/all"
* bucketremove "address" "port"
* connect "address" "port"
* count
* disconnect "address" "port"
* forcerealfake "real" "fake"
* getmsginfo
* list
* listallstats
* listcmpct
* log "category"
* mine ( "duration" "times/seconds/clocks" "delayBetweenNonces" "address" )
* nextIPselect "address"
* send "msg" ( "args" )
* setcmpct

A new optional parameter has also been added to the configuration, named `numconnections`. This can be used by adding `numconnections=X` to ~/.bitcoin/bitcoin.conf, where X is the number of peer connections you would like to add. This overrides the previous "maxconnections" parameter with a more powerful function that forces the peer to connect to many more connections than they default 10 peers (even when incoming connections are disabled). 

For additional information about how to use each command, as well as a brief description of what each one does, enter `help` followed by the command in question.

Questions, comments, concerns: message me.
