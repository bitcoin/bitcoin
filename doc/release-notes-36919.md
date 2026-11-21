New settings
------------

- A new `-staletips=<mode>` option tracks recent stale chain tips for
  stale-tip relay (BIP 332). The modes are `none` (the default) and
  `headers`. (#36919)

Updated RPCs
------------

- `getchaintips` has a new optional `options` argument. Its `max_depth`
  option limits the result to tips at most that many blocks below the
  active tip, and its `staletips_only` option lists only the recent stale
  tips tracked with `-staletips`. The options can also be passed as named
  arguments. (#36919)

- `getchaintips` reports `fork_point`, the block where the branch
  connects to the active chain, for each non-active tip. (#36919)
