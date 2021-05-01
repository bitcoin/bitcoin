### Rust Cuckoo Filter

Replaces the rolling bloom filter used for [`m_addr_known`](https://github.com/bitcoin/bitcoin/blob/13f24d135b280a9ab947f7948f6d86f00104cde1/src/net.h#L550) with a [Cuckoo Filter](https://github.com/axiomhq/rust-cuckoofilter) written in Rust.

Cuckoo filters have some advantages over Bloom filters, including the ability to remove items.

See ["Cuckoo Filter: Practically Better Than Bloom"](https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf):

> In many networking systems, Bloom filters are used for high-speed set membership tests. They permit a small fraction of false positive answers with very good space efficiency. However, they do not permit deletion of items from the set, and previous attempts to extend “standard” Bloom filters to support deletion all degrade either space or performance.

> We propose a new data structure called the cuckoo filter that can replace Bloom filters for approximate set member- ship tests. Cuckoo filters support adding and removing items dynamically while achieving even higher performance than Bloom filters. 

> For applications that store many items and target moderately low false positive rates, cuckoo filters have lower space overhead than space-optimized Bloom filters. Our experimental results also show that cuckoo filters out-perform previous data structures that extend Bloom filters to support deletions substantially in both time and space.

Requires [`Cargo`](https://doc.rust-lang.org/cargo/) to be installed.
```bash
./autogen.sh
./configure --enable-experimental-rust
make
make -C src rusty-check
src/bitcoind
```
