### Policy

The maximum number of potentially executed legacy signature operations in a
single standard transaction is now limited to 2500. Signature operations in all
previous output scripts, in all input scripts, as well as all P2SH redeem
scripts (if there are any) are counted toward the limit. The new limit is
assumed to not affect any known typically formed standard transactions. The
change was done to prepare for a possible BIP54 deployment in the future.
(#32521)
