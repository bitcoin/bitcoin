P2P

- The transaction orphanage, which holds transactions with missing inputs
  temporarily while the node attempts to fetch its parents, now has improved
Denial of Service protections. Previously, it enforced a maximum number of
unique transactions (default 100, configurable using `-maxorphantxs`). Now, it
limits the total memory usage to 404,000Wu multiplied by the number of peers
and 3,000 total entries (each entry identified by its transaction and
announcing peer id).
- The `-maxorphantxs` option has been removed, since the orphanage no longer
  limits the number of unique transactions.
