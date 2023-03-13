Tools and Utilities
--------

- CLI -addrinfo previously (v22 - v29) returned addresses known to the node after filtering for quality and recency.
  However, the node considers all known addresses (even the filtered addresses) when selecting peers to connect to.
  Update -addrinfo to also return the full set of known addresses for more useful node information. (#26988)
