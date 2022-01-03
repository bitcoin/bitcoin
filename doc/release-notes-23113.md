Notable changes
===============

Updated RPCs
------------

- Both `createmultisig` and `addmultisigaddress` now include a `warnings`
field, which will show a warning if a non-legacy address type is requested
when using uncompressed public keys.
