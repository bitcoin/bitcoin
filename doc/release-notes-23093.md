Notable changes
===============

Updated RPCs
------------

- `upgradewallet` will now automatically flush the keypool if upgrading
from a non-HD wallet to an HD wallet, to immediately start using the
newly-generated HD keys.
- a new RPC `newkeypool` has been added, which will flush (entirely
clear and refill) the keypool.
