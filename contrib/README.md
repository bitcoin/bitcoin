Repository Tools
---------------------

### [Developer tools](/contrib/devtools) ###
Specific tools for developers working on this repository.
Contains the script `github-merge.py` for merging GitHub pull requests securely and signing them using GPG.

### [Verify-Commits](/contrib/verify-commits) ###
Tool to verify that every merge commit was signed by a developer using the above `github-merge.py` script.

### [Linearize](/contrib/linearize) ###
Construct a linear, no-fork, best version of the blockchain.

### [Qos](/contrib/qos) ###

A Linux bash script that will set up traffic control (tc) to limit the outgoing bandwidth for connections to the Bitcointalkcoin network. This means one can have an always-on bitcointalkcoind instance running, and another local bitcointalkcoind/bitcointalkcoin-qt instance which connects to this node and receives blocks from it.

### [Seeds](/contrib/seeds) ###
Utility to generate the pnSeed[] array that is compiled into the client.

Build Tools and Keys
---------------------

### Packaging ###
The [Debian](/contrib/debian) subfolder contains the copyright file.

All other packaging related files can be found in the [bitcointalkcoin-core/packaging](https://github.com/bitcointalkcoin-core/packaging) repository.

### [Gitian-descriptors](/contrib/gitian-descriptors) ###
Files used during the gitian build process. For more information about gitian, see the [the Bitcointalkcoin Core documentation repository](https://github.com/bitcointalkcoin-core/docs).

### [Gitian-keys](/contrib/gitian-keys)
PGP keys used for signing Bitcointalkcoin Core [Gitian release](/doc/release-process.md) results.

### [MacDeploy](/contrib/macdeploy) ###
Scripts and notes for Mac builds. 

### [Gitian-build](/contrib/gitian-build.py) ###
Script for running full Gitian builds.

Test and Verify Tools 
---------------------

### [TestGen](/contrib/testgen) ###
Utilities to generate test vectors for the data-driven Bitcointalkcoin tests.

### [Verify Binaries](/contrib/verifybinaries) ###
This script attempts to download and verify the signature file SHA256SUMS.asc from bitcointalkcoin.org.
