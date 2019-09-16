Pruning
-------

When using assumeutxo with `-prune`, the prune budget may be exceeded if it is set
lower than 1100MB (i.e. `MIN_DISK_SPACE_FOR_BLOCK_FILES * 2`). Prune budget is normally
split evenly across each chainstate, unless the resulting prune budget per chainstate
is beneath `MIN_DISK_SPACE_FOR_BLOCK_FILES` in which case that value will be used.
