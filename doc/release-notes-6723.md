Updated RPCs
------------

* The RPCs `protx register_legacy`, `protx register_fund_legacy`, `protx register_prepare_legacy` and
  `protx update_registrar_legacy` have been deprecated in Dash Core v23 and may be removed in a future version
  They can be re-enabled with the runtime argument `-deprecatedrpc=legacy_mn`.

* The argument `legacy` in `bls generate` has been deprecated in Dash Core v23 and may be ignored in a future version.
  It can be re-enabled with the runtime argument `deprecatedrpc=legacy_mn`.
