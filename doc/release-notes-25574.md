Updated settings
----------------

If the `-checkblocks` or `-checklevel` options are explicitly provided by the
user, but the verification checks cannot be completed due to an insufficient
dbcache, Bitcoin Core will now return an error at startup. (#25574)

RPC
---
The `-verifychain` RPC will now return `false` if the checks didn't fail,
but couldn't be completed at the desired depth and level. This could be due
to missing data while pruning, due to an insufficient dbcache or due to
the node being shutdown before the call could finish. (#25574)
