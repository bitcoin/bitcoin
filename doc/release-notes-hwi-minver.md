Wallet
------

As of the introduction of PSBTv2 support in 32.0, Bitcoin Core is no longer
compatible with external signer software that does not support PSBTv2. This does
not mean users need to have hardware wallets that support PSBTv2, only that
users that start bitcoind with an external signer set via `-signer=<cmd>`, will
need to be using an external signing tool, like HWI, which supports PSBTv2. For
users of HWI, this means a minimum version of 2.1.0 (released in 2022) is
required. (https://github.com/bitcoin-core/HWI/releases/tag/2.1.0)
