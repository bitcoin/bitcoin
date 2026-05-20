Logging
-------

- BIP 9 bits 5 to 28 inclusive are now ignored for soft fork signaling, as per BIP 323. We won't
  warn about unknown deployments when receiving blocks that set any of those bits in their version.
