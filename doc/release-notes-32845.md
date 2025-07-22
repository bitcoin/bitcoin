Updated RPCs
------------

- `unloadwallet` - Return RPC_INVALID_PARAMETER when both the RPC wallet endpoint
and wallet_name parameters are unspecified. Previously the RPC failed with a JSON
parsing error.
- `getdescriptoractivity` - Mark blockhashes and scanobjects arguments as required,
so the user receives a clear help message when either is missing. As in `unloadwallet`,
previously the RPC failed with a JSON parsing error.
