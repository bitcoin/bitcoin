RPC changes
-----------
The RPC `getwalletinfo` response now includes the `scanning` key with an object
if there is a scanning in progress or `false` otherwise. Currently the object
has the scanning duration and progress.
