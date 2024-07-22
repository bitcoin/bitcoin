Mainnet Spork Hardening
-----------------------

This version hardens all Sporks on mainnet. Sporks remain in effect on all devnets and testnet; however, on mainnet,
the value of all sporks are hard coded to 0, or 1 for SPORK_21_QUORUM_ALL_CONNECTED. These hardened values match the
active values historically used on mainnet, so there is no change in the network's functionality.
