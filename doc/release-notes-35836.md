# RPC (wallet)

* The `fundrawtransaction` RPC no longer accepts a boolean as the second
  positional argument. This silent no-op fallback was removed and the argument
  is now fully type checked. Passing a boolean will raise an error. (#35836)
