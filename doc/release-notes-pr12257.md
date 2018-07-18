Notable changes
===============

Coin selection
--------------
- A new `-avoidpartialspends` flag has been added (default=false). If enabled, the wallet will try to spend UTXO's that point at the same destination
  together. This is a privacy increase, as there will no longer be cases where a wallet will inadvertently spend only parts of the coins sent to
  the same address (note that if someone were to send coins to that address after it was used, those coins will still be included in future
  coin selections).
