Statistics
-----------

### New Features

- The Statsd client now supports queueing and batching messages, reducing the number of packets and the rate at which
  they are sent to the Statsd daemon.

- The maximum size of each batch of messages (default, 1KiB) can be adjusted using `-statsbatchsize` (in bytes)
  and the frequency at which queued messages are sent to the daemon (default, 1 second) can be adjusted using
  `-statsduration` (in milliseconds)
  - `-statsduration` has no bearing on `-statsperiod`, which dictates how frequently some stats are _collected_.

### Deprecations

- `-statsenabled` has been deprecated and enablement will now be implied by the presence of `-statshost`. `-statsenabled`
  will be removed in a future release.

- `-statshostname` has been deprecated and replaced with `-statssuffix` as the latter is better representative of the
  argument's purpose. They behave identically to each other. `-statshostname` will be removed in a future
  release.

- `-statsns` has been deprecated and replaced with `-statsprefix` as the latter is better representative of the
  argument's purpose. `-statsprefix`, unlike `-statsns`, will enforce the usage of a delimiter between the prefix
  and key. `-statsns` will be removed in a future release.
