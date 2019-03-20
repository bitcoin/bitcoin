# Crown 0.13.0 Release Notes

Crown Core tree 0.13.x release notes can be found here:
- [v0.13.0](release-notes/crown/release-notes-0.13.0.md)

Crown Core tree 0.13.x was originally forked from Bitcoin Core tree 0.10.2

Bitcoind can now (optionally) asynchronously notify clients through a
ZMQ-based PUB socket of the arrival of new transactions and blocks.
This feature requires installation of the ZMQ C API library 4.x and
configuring its use through the command line or configuration file.
Please see docs/zmq.md for details of operation.
>>>>>>> 6e18268... Switch to libsecp256k1-based validation for ECDSA
