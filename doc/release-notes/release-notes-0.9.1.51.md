Credits Core version 0.9.1.51 is now available from:

  http://credits-currency.org

How to Upgrade
--------------

NOTE! Version 0.51 continues the name adaption from Bitcredit to Credits. This involves
renaming all internal names in the working directory. All bitcredit_* files in the 
working directory are renamed automatically BUT the working directory itself may have
to be renamed manually. 

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

0.9.1.51 Release notes
=======================

0.9.1.51 is a stability and renaming release:
- Version number bumped to *.51.
- Major stability improvements have been done in this release, specifically to address the problematic "claim tip" error.
- Working directory file names automatically updated from bitcredit_* to credits_*.
- Continued renaming of internal code base from Bitcredit to Credits.
