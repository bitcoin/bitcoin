RPC
---

A new field `cpu_load` has been added to the `getpeerinfo` RPC output.
It shows the CPU time (user + system) spent processing messages from the
given peer and crafting messages for it expressed in per milles (‰) of
the duration of the connection. The field is optional and will be omitted
on platforms that do not support this or if still not measured. (#31672)
