Updated settings
----------------

- Several numeric options now reject values outside the range they are stored
  in, instead of silently wrapping to a huge value, inverting, or falling back
  to the default. Configurations that start on previous releases can therefore
  fail to start after upgrading; check for these options in `bitcoin.conf` and
  the command line if a node reports one of the new errors. Examples that used
  to be accepted are `-timeout=0`, `-bantime=0`, `-limitclustersize=0`,
  `-limitclustercount=0`, and `-txconfirmtarget=0`, or above 1008 when a wallet
  is loaded.

  - Options that now require a value greater than zero:

    - `-bantime`: zero or negative previously produced bans that had already
      expired.
    - `-timeout`: zero or negative previously fell back to the default
      silently. The value is in milliseconds and must not exceed 2147483647.
    - `-limitclustersize`: zero made every transaction exceed the cluster size
      limit. Negative values wrapped to an effectively unlimited limit. The
      maximum is 9223372036854775 kilobytes.
    - `-limitclustercount`: the accepted range is 1 to 64.
    - `-txconfirmtarget`: the accepted range is 1 to 4294967295, and it is also
      capped at the highest confirmation target the fee estimator tracks (1008
      by default) when a wallet is loaded. Previously a target above that range
      made every transaction fall back to `-fallbackfee`. `-blocksonly` starts
      no fee estimator, so the estimator cap does not apply in that mode.

  - Options that still accept zero, which keeps its existing meaning of
    "disabled" or "unlimited", but now reject negative and oversized values:

    - `-maxtipage`: at most 4294967295 seconds. A negative value placed the
      initial block download cutoff in the future, so the node never treated
      its tip as recent.
    - `-mempoolexpiry`: at most 2562047788015215 hours. A negative value
      expired ordinary transactions instead of keeping them.
    - `-dbbatchsize` and `-dbcrashratio`: `-dbbatchsize` is at most
      18446744073709551615 bytes and `-dbcrashratio` at most 4294967295. A
      negative `-dbbatchsize` disabled batch flushing.
    - `-datacarriersize` and `-bytespersigop`: at most 4294967295. Negative
      values previously wrapped and, for `-bytespersigop`, made every
      transaction non-standard.
    - `-maxsendbuffer` and `-maxreceivebuffer`: at most 4294967, since the
      value is multiplied by 1000 to give bytes.
    - `-limitancestorcount` and `-limitdescendantcount`.

- Negating a setting (`-nofoo`) has always been read as setting it to zero.
  For `-dbbatchsize`, `-mempoolexpiry`, `-maxreceivebuffer`, `-maxsendbuffer`,
  `-maxtipage`, `-limitancestorcount`, `-limitdescendantcount` and
  `-datacarriersize`, zero is valid but means something very different from
  "no limit", so `-nofoo` is now rejected instead of quietly turning the limit
  off. The options that already reject zero report the same error for `-nofoo`
  as for `-foo=0`. `-bytespersigop` still accepts `-nobytespersigop`, because
  zero turns the sigop adjustment off, which is what the negation says.

- `-maxtipage`, `-dbbatchsize`, `-dbcrashratio` and `-limitclustercount` are
  now parsed strictly, so a value with trailing characters such as
  `-maxtipage=1h` is rejected instead of being truncated to the leading number.

These changes were implemented via PR (#36861)
