# Various test vectors

## mainnet_alt.json

For easier testing the difficulty is maximally increased in the first (and only)
retarget period, by producing blocks approximately 2 minutes apart.

The alternate mainnet chain was generated as follows:
- use faketime to set node clock to 2 minutes after genesis block
- mine a block using a CPU miner such as https://github.com/pooler/cpuminer
- restart node with a faketime 2 minutes later

```sh
for i in {1..2016}
do
 t=$(( 1231006505 + $i * 120 ))
 faketime "`date -d @$t  +'%Y-%m-%d %H:%M:%S'`" \
 bitcoind -connect=0 -nocheckpoints -stopatheight=$i
done
```

The CPU miner is kept running as follows:

```sh
./minerd -u ... -p ... -o http://127.0.0.1:8332 --no-stratum \
        --coinbase-addr 1NQpH6Nf8QtR2HphLRcvuVqfhXBXsiWn8r \
        --algo sha256d --no-longpoll --scantime 3 --retry-pause 1
```

The payout address is derived from first BIP32 test vector master key:

```
pkh(xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/44h/0h/0h/<0;1>/*)#fkjtr0yn
```

It uses `pkh()` because `tr()` outputs at low heights are not spendable (`unexpected-witness`).

This makes each block deterministic except for its timestamp and nonce, which
are stored in `mainnet_alt.json` and used to reconstruct the chain without
having to redo the proof-of-work.

The timestamp was not kept constant because at difficulty 1 it's not sufficient
to only grind the nonce. Grinding the extra_nonce or version field instead
would have required additional (stratum) software. It would also make it more
complicated to reconstruct the blocks in this test.

The `getblocktemplate` RPC code needs to be patched to ignore not being connected
to any peers, and to ignore the IBD status check.

On macOS use `faketime "@$t"` instead.
