Updated RPCs
------------

- The `validateaddress` RPC now returns an `error_locations` array for invalid
addresses, with the indices of invalid character locations in the address (if
known). For example, this will attempt to locate up to two Bech32 errors, and
return their locations if successful. Success and correctness are only guaranteed
if fewer than two substitution errors have been made.
The error message returned in the `error` field now also returns more specific
errors when decoding fails.