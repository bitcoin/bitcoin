src/test/fee_est -- Fee estimation offline testing tool
=======================================================

The `fee_est` tool is intended to help debug and test changes in bitcoin fee
estimation code using transaction data gathered from live bitcoin nodes.

Transaction data can be collected by running bitcoind with `-estlog` parameter
which will produce newline-delimited json file
([ndjson.org](http://ndjson.org/), [jsonlines.org](http://jsonlines.org/))
which logs relevant transaction information. The `fee_est` tool can parse the
log to produce graphs of fee estimation data and do some basic analysis of the
results. The implementation is intended to be simple enough that it can be
easily customized to do more specific analysis.

Example usage:

Run bitcoind collecting fee estimation data:

```
$ make -C src bitcoind test/fee_est/fee_est
$ src/bitcoind -estlog=est.log &
```

Run fee estimation tool producing graph of estimates:

```
$ src/test/fee_est/fee_est -ograph=graph.html est.log
$ chrome graph.html
```

Run fee estimation tool computing some basic statistics:

```
$ src/test/fee_est/fee_est -cross est.log
Non-test txs: 1407226
Test txs: 5603 total (4345 kept, 631 discarded unconfirmed, 627 discarded outliers)
Mean squared error: 71.0651
```
