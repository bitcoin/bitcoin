xbitd and XBit-Qt version 0.5.5 are now available for download at:
Windows: installer | zip (sig)
Source: tar.gz
xbitd and XBit-Qt version 0.6.0.7 are also tagged in git, but it is recommended to upgrade to 0.6.1.

These are bugfix-only releases.

Please report bugs by replying to this forum thread. Note that the 0.4.x wxXBit GUI client is no longer maintained nor supported. If someone would like to step up to maintain this, they should contact Luke-Jr.

BUG FIXES

Version 0.6.0 allowed importing invalid "private keys", which would be unspendable; 0.6.0.7 will now verify the private key is valid, and refuse to import an invalid one
Verify status of encrypt/decrypt calls to detect failed padding
Check blocks for duplicate transactions earlier. Fixes #1167
Upgrade Windows builds to OpenSSL 1.0.1b
Set label when selecting an address that already has a label. Fixes #1080 (XBit-Qt)
JSON-RPC listtransactions's from/count handling is now fixed
Optimize and fix multithreaded access, when checking whether we already know about transactions
Fix potential networking deadlock
Proper support for Growl 1.3 notifications
Display an error, rather than crashing, if encoding a QR Code failed (0.6.0.7)
Don't erroneously set "Display addresses" for users who haven't explicitly enabled it (XBit-Qt)
Some non-ASCII input in JSON-RPC expecting hexadecimal may have been misinterpreted rather than rejected
Missing error condition checking added
Do not show green tick unless all known blocks are downloaded. Fixes #921 (XBit-Qt)
Increase time ago of last block for "up to date" status from 30 to 90 minutes
Show a message box when runaway exception happens (XBit-Qt)
Use a messagebox to display the error when -server is provided without providing a rpc password
Show error message instead of exception crash when unable to bind RPC port (XBit-Qt)
Correct sign message xbit address tooltip. Fixes #1050 (XBit-Qt)
Removed "(no label)" from QR Code dialog titlebar if we have no label (0.6.0.7)
Removed an ugly line break in tooltip for mature transactions (0.6.0.7)
Add missing tooltip and key shortcut in settings dialog (part of #1088) (XBit-Qt)
Work around issue in boost::program_options that prevents from compiling in clang
Fixed bugs occurring only on platforms with unsigned characters (such as ARM).
Rename make_windows_icon.py to .sh as it is a shell script. Fixes #1099 (XBit-Qt)
Various trivial internal corrections to types used for counting/size loops and warnings
