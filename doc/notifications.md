# Shell Command Notifications

Bitcoin Core can execute user-supplied shell commands when certain events occur,
configured through the `-*notify` startup options. They are only available on
platforms built with shell command support. See the `--help` output for each
option's placeholders and details.

Each notification (except `-shutdownnotify`) runs its command in a new detached
thread, so commands may run concurrently and complete in any order. There is no
ordering guarantee, even within a single topic. Consumers that need consistent
state should query the node (e.g. via RPC) rather than rely on the values passed
to the command.

The available options are:

- `-blocknotify` — when the best block changes
- `-walletnotify` — when a wallet transaction is added or updated
- `-alertnotify` — when a node warning is raised
- `-startupnotify` — once on startup
- `-shutdownnotify` — immediately before shutdown begins
