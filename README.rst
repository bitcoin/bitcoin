Bitcoin-qt: Qt4 based GUI replacement for Bitcoin
=================================================

**Warning** **Warning** **Warning**
Pre-alpha stuff! Use on testnet only!

This has been implemented:

- qmake / QtCreator project (.pro)

- All dialogs (main GUI, address book, send coins) and menus

- Taskbar icon/menu

- GUI only functionality (copy to clipboard, select address, address/transaction filter proxys)

- Bitcoin core is made compatible with Qt4, and linked against

- Send coins dialog: address and input validation

- Address book and transactions views and models

- Sending coins

This has to be done:

- Settings are not remembered between invocations yet

- Minimize to tray / Minimize on close

- Start at system start

- Internationalization (convert WX language files)

- Build on Windows

- Details dialog for transactions (on double click)

