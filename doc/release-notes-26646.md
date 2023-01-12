JSON-RPC
--------

The `testmempoolaccept` RPC now returns 2 additional results within the "fees" result:
"effective-feerate" is the feerate including fees and sizes of transactions validated together if
package validation was used, and also includes any modified fees from prioritisetransaction. The
"effective-includes" result lists the wtxids of transactions whose modified fees and sizes were used
in the effective-feerate (#26646).
