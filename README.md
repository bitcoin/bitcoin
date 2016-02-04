#GameUnits integration/staging tree
================================
Copyright (c) 2009-2011 Bitcoin Developers<br>
Copyright (c) 2011-2013 Novacoin Developers<br>
Copyright (c) 2016-2999 GameUnits Developers<br>

![GameUnits](http://i.imgur.com/DknBlfE.png)

#What is GameUnits?
----------------
[![GameUnits](http://i.imgur.com/Cokp8iC.png)](http://i.imgur.com/Cokp8iC.png)

GameUnits is a PoS-based cryptocurrency.

A new and exciting Open Source Gaming currency that will revolutionize in-game purchases and bring developers a monetization based on fair-play rules.

GameUnits is a lite version of Bitcoin using scrypt as a proof-of-work algorithm with Proof of Stake.

GameUnits Properties :

-

# We :heart: Pull Requests!
Seriously, we really do.  It doesn't matter whether you're fixing a typo or overhauling a major area of the code base.  You will be showered in :thumbsup: :thumbsup: :thumbsup:<br>

#License
-------
![GameUnits](http://i.imgur.com/Nfb8DQx.png)

GameUnits is released under the terms of the MIT license. See `COPYING` for more
information or see http://opensource.org/licenses/MIT.

#Development process
-------------------

Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the GameUnits
development team members simply pulls it.

If it is a *more complicated or potentially controversial* change, then the patch
submitter will be asked to start a discussion (if they haven't already) on the
[mailing list]().

The patch will be accepted if there is broad consensus that it is a good thing.
Developers should expect to rework and resubmit patches if the code doesn't
match the project's coding conventions (see `doc/coding.txt`) or are
controversial.

#Compiling the GameUnits daemon from source on Debian
-----------------------------------------------------
The process for compiling the GameUnits daemon, gameunitsd, from the source code is pretty simple. This guide is based on the latest stable version of Debian Linux, though it should not need many modifications for any distro forked from Debian, such as Ubuntu and Xubuntu.

###Update and install dependencies

```
apt-get update && apt-get upgrade
apt-get install ntp git build-essential libssl-dev libdb-dev libdb++-dev libboost-all-dev libqrencode-dev autoconf automake pkg-config unzip

wget http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.8.tar.gz && tar -zxf download.php\?file\=miniupnpc-1.8.tar.gz && cd miniupnpc-1.8/
make && make install && cd .. && rm -rf miniupnpc-1.8 download.php\?file\=miniupnpc-1.8.tar.gz
```
Note: Debian testing and unstable require libboost1.54-all-dev.

###Compile the daemon
```
git clone https://github.com/GameUnits/GameUnits
```

###Compile gameunitsd
```
cd gameunits/src
make -f makefile.unix
cd src
strip gameunitsd
```

###Compile gameunits-qt
```
-------
```

###Add a user and move gameunitsd
```
adduser gameunits && usermod -g users gameunits && delgroup gameunits && chmod 0701 /home/gameunits
mkdir /home/gameunits/bin
cp ~/gameunits/src/gameunitsd /home/gameunits/bin/gameunitsd
chown -R gameunits:users /home/gameunits/bin
cd && rm -rf gameunits
```

###Run the daemon
```
su gameunits
cd && bin/gameunitsd
```

On the first run, gameunitsd will return an error and tell you to make a configuration file, named gameunits.conf, in order to add a username and password to the file.
```
nano ~/.gameunits/gameunits.conf && chmod 0600 ~/.gameunits/gameunits.conf
```
Add the following to your config file, changing the username and password to something secure: 
```
daemon=1
rpcuser=<username>
rpcpassword=<secure password>
server=1
listen=1
txindex=1
#txindex will record every transaction from the blockchain to your offline db
#it's an optional thing. It takes a lot longerr to sync that way 0 if you don't care
rpcport=40001
port=40002
rpcallowip=127.0.0.1
addnode=51.254.127.4:1338
addnode=95.211.57.108:1338
addnode=5.196.70.166:1338
```

You can just copy the username and password provided by the error message when you first ran gameunitsd.

Run gameunitsd once more to start the daemon! 

###Optional Download gameunits bootstrap
```
cd /home/gameunits/.gameunits/
wget --
unzip bootstrap.zip
```

###Using gameunitsd
```
gameunitsd help
```

The above command will list all available functions of the gameunits daemon. To safely stop the daemon, execute gameunitsd stop. 

#Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test. Please be patient and help out, and
remember this is a security-critical project where any mistake might cost people
lots of money.