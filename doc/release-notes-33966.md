Mining
------

- The IPC mining interface now rejects out-of-range block template options
  instead of silently clamping them, such as oversized reserved block weight or
  coinbase sigops limits. (#33966)

- The `-blockmaxweight` startup option is now rejected when it is lower than
  `-blockreservedweight`, instead of being silently clamped. (#33966)
