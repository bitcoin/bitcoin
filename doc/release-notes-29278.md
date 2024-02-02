wallet startup option
========================

- A new wallet startip option `-maxfeerate` is added.
- This option sets the upper limit for wallet transaction fee rate.
- The wallet will now refrain from creating transactions with a fee rate exceeding the `maxfeerate`.
- The default fee rate is 10,000 satoshis per virtual byte.