
Low-level changes
=================

Tests
---

- `-fallbackfee` was 0 (disabled) by default for the main chain, but 20000 by default for the test chains. Now it is 0 by default for all chains. Testnet and regtest users will have to add fallbackfee=20000 to their configuration if they weren't setting it and they want it to keep working like before. (#16524)
