Bitcoin-qt: Qt4 based GUI replacement for Bitcoin
=================================================

**Warning** **Warning** **Warning**
Pre-alpha stuff! Use on testnet only!

This has been implemented:

- qmake / QtCreator project (.pro)

- All dialogs (main GUI, address book, send coins) and menus

- Taskbar icon/menu

- GUI only functionality (copy to clipboard, select address, address/transaction filter proxys)

- Bitcoin core is made compatible with Qt4

- Send coins dialog: address and input validation

- Address book and transactions views and models

- Options dialog

- Sending coins (including ask for fee when needed)

- Show error messages from core

- Show details dialog for transactions (on double click)

This has to be done:

- Start at system start

- Internationalization (convert WX language files)

- Build on Windows

- More thorough testing of the view with all the kinds of transactions (sendmany, generation)
