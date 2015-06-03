Credits Core version 0.9.1.60 is now available from:

  http://credits-currency.org

How to Upgrade
--------------

NOTE! Starting with version 0.9.1.60 the Bitcoin block chain will be trimmed to preserve
spaces, unless the option bitcoin_trimblockfiles has been set to 0. This trimming requires 
a reindex of the working directory, or a download of a torrent file that can be obtained from
http://credits-currency.org/viewtopic.php?f=18&t=517. The torrent file is 6.5GB. If a reindex
is done, or initialization done from a clean install, the download process will require 22GB
free on disk, which shrinks to about 7.7GB when the syncing of the working directory has finished.

NOTE 2! If you are upgrading from a version of Credits that is lower than 0.9.1.51, the 
working directory could still be called Credits or .credits, depending on operating
system. You have to rename the directory yourself.

The default working directories are:
For Windows: C:/Users/<user home directory>/AppData/Roaming/Bitcredit
For Ubuntu/Linux: <user home directory>/.bitcredit
For Mac: /Users/<user home directory>/Library/Application Support/Bitcredit

And should be renamed to:
For Windows: C:/Users/<user home directory>/AppData/Roaming/Credits
For Ubuntu/Linux: <user home directory>/.credits
For Mac: /Users/<user home directory>/Library/Application Support/Credits

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Credits-Qt (on Mac) or
creditsd/credits-qt (on Linux).

!!!BEFORE YOU START THE WALLET, MAKE SURE TO MAKE BACKUP COPIES OF THE FILES BITCREDIT_WALLET.DAT
AND BITCOIN_WALLET.DAT, IF YOU HAVE A PREVIOUS INSTALLATION.!!!

Start credits-qt or creditsd with the command:
credits-qt -reindex -checklevel=2

0.9.1.60 Release notes
=======================

0.9.1.60 is a major release, further improving the stability, and trimming the blockchain to preserve disk space
- Version number bumped to *.60.
- Further stability improvements have been done  to address the problematic "claim tip" error.
  The working directory bitcoin_chainstatecla will be merged with the credits_chainstate directory.
  This change requires a reindex of the working directory, or downloading an up to date 
  version of the working directory from a torrent. 
- The Bitcoin blockchain will be trimmed once all relevant blocks has been parsed. This will save 
  a substantial amount of space on disk, approximately 7.7GB instead of 45GB.
- Continued renaming of internal code base from Bitcredit to Credits.
