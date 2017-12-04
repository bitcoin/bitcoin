# File Partition:
Since the blockchain is around ~140GB, storage of large files on an external drive is convenient.  If this is not done properly, the external drive will also contain high i/o-frequency LevelDB index files, protracting time for initial blockchain synchronization. This document describes how partition datadir files between the high-frequency/low-capacity "index" files and the low-frequency/high-capacity "blocks" files. Examples are given for macOS, but Linux / Windows should be similar. These instructions result in the following physical folder rearrangement:

| Original Location       | New Location | Capacity Needs | i/o Frequency  |
| ----------------------- | ------------ | -------------- | -------------- |
| ${datadir}/blocks/index | ${datadir}   | low            | high           |
| ${datadir}/blocks       | ${EXTERNAL}  | high           | low            |
| ${datadir}/chainstate   | n/a          | low            | high           |

Note: the chainstate folder will not move if these instructions are followed closely.  Like the index folder, chainstate contains very high frequency LevelDB files and must remain on a fast (internal) disk if reasonable syncronization time is to be expected.

Change "coinadm" in the following as appropriate for your Bitcoin administrative account.

1) Start bitcoind with -datadir pointing to your internal drive:

        bitcoind -daemon -conf=macos-bitcoin.conf -datadir=/Users/coinadm/local/bitcoin/data

Let it run for a few minutes, or long enough that it has started syncho-
   nizing.  Either tail the debug.log, or watch the blocks folder:
   
        ls -l /Users/coinadm/local/bitcoin/data/blocks/blk00000.dat
        -rw-------  1 coinadm  staff  134191893 Jul 27 11:31 /Users/coinadm/local/data/blocks/blk00000.dat

2) Stop bitcoind, so that we can rearrange some datadir folders:

       bitcoin-cli stop

3) Move the "index" folder up one level, so that it may remain on the internal drive: 
      
          mv -f /Users/coinadm/local/bitcoin/data/blocks/index /Users/coinadm/local/bitcoin/data/

4) Next, move the Bitcoin blockchain blocks folder off the internal drive:
   
          mkdir /Volumes/WD-Passport-Mac/bitcoin 
          mv -f /Users/coinadm/local/bitcoin/data/blocks /Volumes/WD-Passport-Mac/bitcoin 

Finally, set up soft links to restore the original folder structure:
   
| Folder Name              | Link Name           |
| ------------------------ | ------------------- |
| ${EXTERNAL}/blocks/index | ${datadir}/../index |
| ${datadir}/blocks        | ${EXTERNAL}/blocks  |

5) Replace the original index folder location with a soft link:
      
          ln -s /Users/coinadm/local/bitcoin/index /Volumes/WD-Passport-Mac/bitcoin/blocks/index 

6) Replace the original blocks folder location with a soft link:
      
          ln -s /Volumes/WD-Passport-Mac/bitcoin/blocks /Users/coinadm/local/bitcoin/data/blocks

 注意 - Nota - Note - ध्यान दें - ﻢﻠﺣﻮﻇﺓ - метка 
These instructions are confusing due to the way the LevelDB "index" folder is nested within the blockchain "blocks" folder by default.
