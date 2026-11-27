RPC
---

A new field `cpu_load` has been added to the `getpeerinfo` RPC output. It
shows the CPU time (user + system) spent processing messages to/from the
peer, in per milles (‰) of the duration of the connection. The field is
optional and will be omitted on platforms that do not support this or if not
yet measured. Note that high CPU time is not necessarily a bad thing - new
valid transactions and blocks require CPU time to be validated. (#31672)
