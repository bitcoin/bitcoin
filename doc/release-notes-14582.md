Configuration
-------------

A new configuration flag `-maxapsfee` has been added, which sets the max allowed
avoid partial spends (APS) fee. It defaults to 0 (i.e. fee is the same with
and without APS). Setting it to -1 will disable APS, unless `-avoidpartialspends`
is set. (#14582)

Wallet
------

The wallet will now avoid partial spends (APS) by default, if this does not result
in a difference in fees compared to the non-APS variant. The allowed fee threshold
can be adjusted using the new `-maxapsfee` configuration option. (#14582)
