HTTP: RPC / REST
----------------

The HTTP server has been rewritten from scratch to replace libevent. (#35182)

The `libevent` logging category has been removed. Configurations like
`-debug=libevent` or `-debugexclude=libevent` will log a deprecation warning
and be ignored. These configurations will result in an error in a future release.

Certain HTTP edge cases will observe different behavior to be more RFC-compliant:

- Stricter enforcement of maximum headers size (8192 bytes)
- Reject requests with whitespace in header field-names
- "Line Folding" is rejected (whitespace at start of a header line)
- Tolerate `%` at the end of requested URLs
- Multiple "Content-Length" headers with different values are rejected
