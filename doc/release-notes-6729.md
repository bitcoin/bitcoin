Notable Changes
---------------

* Dash Core will no longer permit the registration of new legacy scheme masternodes after the deployment of the v23
  fork. Existing basic scheme masternodes will also be prohibited from downgrading to the legacy scheme after the
  deployment is active.

Updated RPCs
----------------

* `protx revoke` will now use the legacy scheme version for legacy masternodes instead of the defaulting to the
   highest `ProUpRevTx` version.
