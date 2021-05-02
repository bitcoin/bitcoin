This folder contains lint scripts.

check-doc.py
============
Check for missing documentation of command line options.

commit-script-check.sh
======================
Verification of [scripted diffs](/doc/developer-notes.md#scripted-diffs).
Scripted diffs are only assumed to run on the latest LTS release of Ubuntu. Running them on other operating systems
might require installing GNU tools, such as GNU sed.

git-subtree-check.sh
====================
Run this script from the root of the repository to verify that a subtree matches the contents of
the commit it claims to have been updated to.

To use, make sure that you have fetched the upstream repository branch in which the subtree is
maintained:
* for `src/secp256k1`: https://github.com/xbit-core/secp256k1.git (branch master)
* for `src/leveldb`: https://github.com/xbit-core/leveldb.git (branch xbit-fork)
* for `src/univalue`: https://github.com/xbit-core/univalue.git (branch master)
* for `src/crypto/ctaes`: https://github.com/xbit-core/ctaes.git (branch master)
* for `src/crc32c`: https://github.com/google/crc32c.git (branch master)

To do so, add the upstream repository as remote:

```
git remote add --fetch secp256k1 https://github.com/xbit-core/secp256k1.git
```

Usage: `git-subtree-check.sh DIR (COMMIT)`

`COMMIT` may be omitted, in which case `HEAD` is used.

lint-all.sh
===========
Calls other scripts with the `lint-` prefix.
