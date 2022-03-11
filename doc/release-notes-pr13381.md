RPC importprivkey: new label behavior
-------------------------------------

Previously, `importprivkey` automatically added the default empty label
("") to all addresses associated with the imported private key.  Now it
defaults to using any existing label for those addresses.  For example:

- Old behavior: you import a watch-only address with the label "cold
  wallet".  Later, you import the corresponding private key using the
  default settings.  The address's label is changed from "cold wallet"
  to "".

- New behavior: you import a watch-only address with the label "cold
  wallet".  Later, you import the corresponding private key using the
  default settings.  The address's label remains "cold wallet".

In both the previous and current case, if you directly specify a label
during the import, that label will override whatever previous label the
addresses may have had.  Also in both cases, if none of the addresses
previously had a label, they will still receive the default empty label
("").  Examples:

- You import a watch-only address with the label "temporary".  Later you
  import the corresponding private key with the label "final".  The
  address's label will be changed to "final".

- You use the default settings to import a private key for an address that
  was not previously in the wallet.  Its addresses will receive the default
  empty label ("").
