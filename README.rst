Bitcoin-qt: Qt4 based GUI replacement for Bitcoin
=================================================

**Warning** **Warning** **Warning**
Pre-alpha stuff! Developer only!

This has been implemented:

- qmake / QtCreator project (.pro)

- All dialogs (main GUI, address book, send coins) and menus

- Taskbar icon/menu

- GUI only functionality (copy to clipboard, select address, address/transaction filter proxys)

- Bitcoin core is made compatible with Qt4, and linked against

- Send coins dialog: address and input validation

- Address book and transactions views

This has to be done:

- Further integration of bitcoin core, so that it can actually connect
  to the network, send commands and get feedback

- Address book and transactions models: so that 
  something sensible is shown instead of dummy data :)

- Internationalization (convert WX language files)

- Build on Windows
