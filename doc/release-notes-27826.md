- Logs now include which peer sent us a header. Additionaly there are fewer
  redundant header log messages. A side-effect of this change is that for
  some untypical cases new headers aren't logged anymore, e.g. a direct
  `BLOCK` message with a previously unknown header and `submitheader` RPC. (#27826)
