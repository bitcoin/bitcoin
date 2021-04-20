Updated RPCs
------------

- The `validateaddress` RPC now optionally returns an `error_locations` array, with the indices of
invalid characters in the address. For example, this will return the locations of up to two Bech32
errors.