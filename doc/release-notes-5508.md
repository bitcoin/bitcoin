Updated RPCs
--------

- `protx register_hpmn` is renamed to `protx register_evo`.
- `protx register_fund_hpmn` is renamed to `protx register_fund_evo`.
- `protx register_prepare_hpmn` is renamed to `protx register_prepare_evo`.
- `protx update_service_hpmn` is renamed to `protx update_service_evo`.

Please note that _hpmn RPCs are deprecated.
Although, they can still be enabled by adding `-deprecatedrpc=hpmn`.

- `masternodelist` filter `hpmn` is renamed to `evo`.
- `masternode count` return total number of EvoNodes under field `evo` instead of `hpmn`.
-  Description type string `HighPerformance` is renamed to `Evo`: affects most RPCs return MNs details.