GUI Changes
-----------
- In 0.18.0 a `./configure` flag was introduced to allow disabling BIP70 support in the GUI (support was enabled by default). In 0.19.0 this flag is now __disabled__ by default.
- If you want compile Bitcoin Core with BIP70 support in the GUI, you can pass `--enable-bip70` to `./configure`.