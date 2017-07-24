Since the blockchain is around 130GB many developers may find it 
convenient to store the blockchain on an external drive.  This
document describes how to partitioning of datadir files between 
the high-frequency/low-capacity "index" files and the low-frequency/high-capacity "blocks" files. 
Examples are given for macOS, but Linux / Windows should be similar. 

Change "username" in the follows as appropriate for your macOS Bitcoin
administrative account ("coinadm" in this example conf file).

1) Start bitcoind with datadir pointing to your internal drive:

        bitcoind -daemon -conf=macos-bitcoin.conf -datadir=/Users/coinadm/local/bitcoin/data
   Let it run for a few minutes, or long enough that it has started syncho-
   nizing.  Either tail the debug.log, or watch the blocks folder:
   
        ls -l /Users/coinadm/local/bitcoin/data/blocks/blk00000.dat
        -rw-------  1 coinadm  staff  134191893 Jul 27 11:31 /Users/coinadm/local/data/blocks/blk00000.dat
2) Stop bitcoind, so that we can rearrange some datadir folders:

       kill -QUIT `cat /Volumes/WD-Passport-Mac/bitcoin/data/bitcoind.pid`
3) Rearrange the datadir folders as follows:

| Original Location       | New Location | Capacity Needs | IO Frequency        |
| ----------------------- | ------------ | -------------- | ------------------- |
| ${datadir}/blocks/index | ${datadir}   | low            | high                |
| ${datadir}/blocks       | ${EXTERAL}   | high           | low                 |

   a) Move the Bitcoin LevelDB index folder up one level (so that it may
      remain on the internal drive).
      
          mv -f /Users/coinadm/local/bitcoin/data/blocks/index /Users/coinadm/local/bitcoin/data/
   b) Next, move the Bitcoin blockchain blocks folder off the internal drive:
   
          mkdir /Volumes/WD-Passport-Mac/bitcoin 
          mv -f /Users/coinadm/local/bitcoin/data/blocks /Volumes/WD-Passport-Mac/bitcoin 
   Finally, set up soft links to restore the original folder structure:
   
| Folder Name              | Link Name           |
| ------------------------ | ------------------- |
| ${EXTERNAL}/blocks/index | ${datadir}/../index |
| ${datadir}/blocks        | ${EXTERAL}/blocks   |

    c) Replace the original index folder location with a soft link:
      
          ln -s /Users/coinadm/local/bitcoin/index /Volumes/WD-Passport-Mac/bitcoin/blocks/index 
    d) Replace the original index folder location with a soft link:
      
          ln -s /Volumes/WD-Passport-Mac/bitcoin/blocks /Users/coinadm/local/bitcoin/data/blocks

 注意 - Nota - Note - ध्यान दें - ﻢﻠﺣﻮﻇﺓ - метка 
These instructions are confusing due to the way the index folder is nested within the blocks folder.
