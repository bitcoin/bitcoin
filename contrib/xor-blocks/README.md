# Bitcoin Core blocks directory XOR obfuscator

Bitcoin Core v28 and later versions have the ability to obfuscate the blocks directory while it is stored on disk.
However, this only takes effect if the blocks directory is fresh, i.e. you will have to resync the node from scratch.

This program will obfuscate the blocks directory without having to redownload the blocks. It creates a random
obfuscation key and XORs all block files. It only works for mainnet.

## Running

Shut down bitcoind and run `cargo run --release`. This looks for blocks in the default location.
If the data or blocks directory is in a different location, pass the location as the next argument:
`cargo run --release -- -datadir=/home/user/.bitcoin`.

This can take some time, so please be patient.

The program can be safely stopped and restarted until all files
have been obfuscated.
