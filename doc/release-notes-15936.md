GUI changes
-----------

Configuration changes made in the bitcoin GUI (such as the pruning setting,
proxy settings, UPNP preferences) are now saved to <datadir>/settings.json file
rather than to the Qt settings backend (windows registry or unix desktop config
files), and the GUI settings will now be used if bitcoind is started in
subsequently, rather than ignored.
