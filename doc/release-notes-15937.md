Configuration
-------------

The `createwallet`, `loadwallet`, and `unloadwallet` RPCs now accept
`load_on_startup` options that modify bitcoin's dynamic configuration in
`\<datadir\>/settings.json`, and can add or remove a wallet from the list of
wallets automatically loaded at startup. Unless these options are explicitly
set to true or false, the load on startup wallet list is not modified, so this
change is backwards compatible.

In the future, the GUI will start updating the same startup wallet list as the
RPCs to automatically reopen wallets previously opened in the GUI.
