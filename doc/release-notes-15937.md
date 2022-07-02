Configuration
-------------

Wallets created or loaded in the GUI will now be automatically loaded on
startup, so they don't need to be manually reloaded next time Bitcoin is
started. The list of wallets to load on startup is stored in
`\<datadir\>/settings.json` and augments any command line or `bitcoin.conf`
`-wallet=` settings that specify more wallets to load. Wallets that are
unloaded in the GUI get removed from the settings list so they won't load again
automatically next startup. (#19754)

The `createwallet`, `loadwallet`, and `unloadwallet` RPCs now accept
`load_on_startup` options to modify the settings list. Unless these options are
explicitly set to true or false, the list is not modified, so the RPC methods
remain backwards compatible. (#15937)
