Updated settings
----------------

- The following negated options now fail at startup: `-noblocksdir`,
  `-nomaxreceivebuffer`, `-nomaxsendbuffer`, `-nocheckblocks` and
  `-nomaxtipage`. `-notorcontrol` now fails while `-listenonion` is enabled.
  Use `-nolistenonion` to stop the node from connecting to the Tor control
  port. `-limitclustercount` and `-limitclustersize` values below 1 are also
  rejected. (#36880)

- `-noalertnotify`, `-noblocknotify`, `-nostartupnotify`, `-nowalletnotify`,
  `-nosigner`, `-noi2psam`, `-notorpassword`, `-norpcuser` and
  `-norpcpassword` now act as an empty value instead of `0`, so they turn off
  a value set earlier, for example in `bitcoin.conf`. Before, the notify
  options and `-nosigner` ran the command `0`, `-noi2psam` failed at startup,
  and `-norpcpassword` set the RPC password to `0`. `-norpcpassword` now
  falls back to cookie authentication. `bitcoin-cli -norpcuser` and
  `bitcoin-cli -norpcpassword` change the same way. (#36880)

- `-cjdnsreachable=0` and `-nocjdnsreachable` no longer make CJDNS reachable.
  (#36880)

- An empty `-blocksdir=` or `-walletdir=` now uses the default directory, as
  if the option were not set. (#36880)
