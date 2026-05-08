HTTP: RPC / REST
----------------

The HTTP server has been rewritten from scratch to replace libevent. (#35182)

`libevent` has been deprecated as a logging category and will be removed in
a future release. At that time configurations like `debugexclude=libevent` will
be invalid.

Certain HTTP edge cases will observe different behavior to be more RFC-compliant:

- Stricter enforcement of maximum headers size (8192 bytes)
- Reject requests with whitespace in header field-names
- "Line Folding" is rejected (whitespace at start of a header line)
- Tolerate `%` at the end of requested URLs
- Multiple "Content-Length" headers with different values are rejected
