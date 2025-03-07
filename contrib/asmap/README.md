# ASMap Tool

Tool for performing various operations on textual and binary asmap files,
particularly encoding/compressing the raw data to the binary format that can
be used in Bitcoin Core with the `-asmap` option.

Example usage:
```
python3 asmap-tool.py encode /path/to/input.file /path/to/output.file
python3 asmap-tool.py decode /path/to/input.file /path/to/output.file
python3 asmap-tool.py diff /path/to/first.file /path/to/second.file
python3 asmap-tool.py diff-addrs /path/to/first.file /path/to/second.file addrs.file
```

### Comparing ASmaps

AS control of IP networks changes frequently, therefore it can be useful to get
the changes between to ASmaps via the `diff` and `diff_addrs` commands.

`diff` takes two ASmap files, and returns a file detailing the changes
in the state of a network's AS assignment between the first and the second file.
The example below shows the three possible output states:
- reassigned to a new AS (`AS26496 # was AS20738`),
- present in the first but not the second (`# 220.157.65.0/24 was AS9723`),
- or present in the second but not the first (`# was unassigned`).

The command accepts a `--ignore-unassigned`/`-i` flag which ignores networks present in the second but not the first.
```
217.199.160.0/19 AS26496 # was AS20738
# 220.157.65.0/24 was AS9723
216.151.172.0/23 AS400080 # was unassigned
2001:470:49::/48 AS20205 # was AS6939
# 2001:678:bd0::/48 was AS207631
2001:67c:308::/48 AS26496 # was unassigned
```

`diff-addrs` is intended to provide changes between two ASmaps and
a node's known peers.
The command takes two ASmap files, and a file of IP addresses as output by
the `bitcoin-cli getnodeaddresses` command.
It returns the changes between the two ASmaps for the peer IPs provided in
the `getnodeaddresses` output.
The resulting file is in the same format as the `diff` command shown above.

You can output address data to a file:
```
bitcoin-cli getnodeaddresses 0 > addrs.json
```
or pipe in the address data via stdin:
```
python3 asmap-tool.py diff-addrs path/to/first.file path/to/second.file < $(bitcoin-cli getnodeaddresses 0)
```
