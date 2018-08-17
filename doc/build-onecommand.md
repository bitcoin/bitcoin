### Instruction
This console command will build last (0.16.2) version Bitcoin clients for Linux 64 bit, Windows 64 bit and Windows 32 bit. 

Has been tested on the clean server with Ubuntu 16 (64 bit 4GB RAM) 

```bash
cd ~ && sudo apt-get update && apt-get upgrade -y && apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev -y && apt-get update -y && add-apt-repository ppa:bitcoin/bitcoin && apt-get update -y && apt-get install libdb4.8-dev libdb4.8++-dev libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev g++-mingw-w64-i686 mingw-w64-i686-dev g++-mingw-w64-x86-64 mingw-w64-x86-64-dev curl -y && update-alternatives --config x86_64-w64-mingw32-g++ &&  wget https://github.com/bitcoin/bitcoin/archive/v0.16.2.tar.gz && tar -xvzf v0.16.2.tar.gz && cd bitcoin-0.16.2/depends && make download && make && cd .. && ./autogen.sh && ./configure --enable-glibc-back-compat --prefix=`pwd`/depends/x86_64-pc-linux-gnu LDFLAGS="-static-libstdc++" && make clean && make && make install && cd depends && make HOST=x86_64-w64-mingw32 -j4 && cd .. && ./configure --prefix=`pwd`/depends/x86_64-w64-mingw32 && make clean && make && make install && cd depends && make HOST=i686-w64-mingw32 -j4 && cd .. && ./configure --prefix=`pwd`/depends/i686-w64-mingw32 && make clean && make && make install
```

First you will asked 
```bash
Press [ENTER] to continue or ctrl-c to cancel adding it
```
you should to press Enter

After some time you will asked
```bash
  Selection    Path                                   Priority   Status
------------------------------------------------------------
* 0            /usr/bin/x86_64-w64-mingw32-g++-win32   60        auto mode
  1            /usr/bin/x86_64-w64-mingw32-g++-posix   30        manual mode
  2            /usr/bin/x86_64-w64-mingw32-g++-win32   60        manual mode

Press <enter> to keep the current choice[*], or type selection number:
```
select "1" for switching to posix and press Enter

After this you will asked

```bash
  Selection    Path                                 Priority   Status
------------------------------------------------------------
* 0            /usr/bin/i686-w64-mingw32-g++-win32   60        auto mode
  1            /usr/bin/i686-w64-mingw32-g++-posix   30        manual mode
  2            /usr/bin/i686-w64-mingw32-g++-win32   60        manual mode

Press <enter> to keep the current choice[*], or type selection number:
```
Again select "1" for switching to posix and press Enter


When command will complete working, Linux binaries will be in this folder ~/bitcoin-0.16.2/depends/x86-64-pc-linux-gnu/bin

Windows binaries will be in this folders: 

~/bitcoin-0.16.2/depends/x86-64-w64-mingw32 (Windows 64 bit binaries)

~/bitcoin-0.16.2/depends/i686-w64-mingw32 (Windows 32 bit binaries)

### Note

If process will killed then may be you have not enough RAM. You can increase it or use this coomand instead

```bash
sudo fallocate -l 4G /swapfile && sudo chmod 600 /swapfile && sudo mkswap /swapfile && sudo swapon /swapfile
```
