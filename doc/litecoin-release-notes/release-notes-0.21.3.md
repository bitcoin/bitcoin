Litecoin Core version 0.21.3 is now available from:

 <https://download.litecoin.org/litecoin-0.21.3/>.

This is a new patch version release that includes, new features and important security updates.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/litecoin-project/litecoin/issues>

Notable changes
===============

Important Security Updates
--------------------------

This release contains fixes for [CVE-2023-33297](https://www.cvedetails.com/cve/CVE-2023-33297/), which allows attacker to cause a remote bandwidth and cpu denial of service. This attack has been exploited in the wild. 
- `2170c24`: backported from Bitcoin Core v24.x

New MWEB features
--------------------------
- `215edcf` - `1049218`: add MWEB light client p2p messages. This implements [LIP006](https://github.com/litecoin-project/lips/blob/c01068d06136fb21bf35fd6dac2977de60057714/LIP-0006.mediawiki). 
- `4c3d4f2`: adds new NODE_MWEB_LIGHT_CLIENT service flag. Enables BIP157/158 blockfilters by default (opt-out by default). 

Build changes
--------------------------
- `bf355d2`: rename the PID file to litecoind.pid
- `5ac7814`: improve build instructions for unix systems
- `24a0299`: fix building with Boost 1.77+
- `a376e2e` - `0698e23`: build changes for macOS. Primarily updates macOS build SDK to Xcode 12.1, increasing minimum macOS version to 10.15.6. These changes fix an issue where Litecoin-Qt UI may not appear as expected on macOS 14+.
- `41b4c16`: fixes builds on Alpine Linux/musl.
- `3b590e9`: hardened runtime build for signed macOS builds

Test related fixes
--------------------------
- `53dbe58`: fix tests, disabling unused test and missing expected values
- `8e16ef3`: fixes macOS test, issue #942.
- `260de84`: run secp256k1-zkp tests. fixes bench test and secp256k1 test.
- `c2a4fc3`: fixes another secp256k1-zkp test
- `c2a4fc3`: disabled run_schnorrsig_tests() in secp256k1-zkp.
- `7e3c1f5`: disable secp256k1-zkp openssl test, fixing `make check` on macOS.

Qt
--------------------------
- `f1c0ecf`: reverts tempfix where macOS 14+ unexpectedly uses Fusion UI theme
- `917312b`: Add context menu option for rebroadcasting unconfirmed transactions

Credits
=======

Thanks to everyone who directly contributed to this release:

- [The Bitcoin Core Developers](https://github.com/bitcoin/bitcoin/)
- [David Burkett](https://github.com/DavidBurkett/)
- [Hector Chu](https://github.com/hectorchu)
- [Loshan](https://github.com/losh11)
- [addman2](https://github.com/addman2)
- [Rafael Sadowski](https://github.com/sizeofvoid)
- [rustyx](https://github.com/rustyx)
- [NamecoinGithub](https://github.com/NamecoinGithub)
- [Patrick Lodder](https://github.com/patricklodder)