Configuration
---

- `syscoind` and `syscoin-qt` will now raise an error on startup if a datadir that is being used contains a syscoin.conf file that will be ignored, which can happen when a datadir= line is used in a syscoin.conf file. The error message is just a diagnostic intended to prevent accidental misconfiguration, and it can be disabled to restore the previous behavior of using the datadir while ignoring the syscoin.conf contained in it.
