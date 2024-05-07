- When running with -alertnotify, an alert can now be raised multiple
times instead of just once. Previously, it was only raised when unknown
new consensus rules were activated, whereas the scope has now been
increased to include all kernel warnings. Specifically, alerts will now
also be raised when an invalid chain with a large amount of work has
been detected. Additional warnings may be added in the future.
(#30058)
