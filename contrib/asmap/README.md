# ASMap Tool

Tool for performing various operations on textual and binary asmap files,
particularly encoding/compressing the raw data to the binary format that can
be used in Bitcoin Core with the `-asmap` option.

Example usage:
```
python3 asmap-tool.py encode /path/to/input.file /path/to/output.file
python3 asmap-tool.py decode /path/to/input.file /path/to/output.file
python3 asmap-tool.py diff /path/to/first.file /path/to/second.file
python3 asmap-tool.py diff_addrs /path/to/first.file /path/to/second.file addrs.file
```
These commands may take a few minutes to run with `python3`,
depending on the amount of data involved and your machine specs.
Consider using `pypy3` for a faster run time.

### Encoding and Decoding

ASmap files are somewhat large in text form, and need to be encoded
to binary before being used with Bitcoin Core.

The `encode` command takes an ASmap and an output file.

The `--fill`/`-f` flag further reduces the size of the output file
by assuming an AS assignment for an unmapped network if an adjacent network is assigned.
This procedure is lossy, in the sense that it loses information
about which ranges were unassigned.
However, if the input ASmap is incomplete,
this procedure will also reassign ranges that should have an AS assignment,
resulting in an ASmap that may diverge from reality significantly.
Finally, another consequence is that the resulting encoded file
will no longer be meaningful for diffs.
Therefore only use `--fill` if
you want to optimise space, you have a reasonably complete ASmap,
and do not intend to diff the file at a later time.

The `decode` command takes an encoded ASmap and an output file.
As with `encode`, the `--fill`/`-f` flag reduces the output file size
by reassigning subnets. Conversely, the `--non-overlapping`/`-n` flag
increases output size by outputting strictly non-overlapping network ranges.

### Comparing ASmaps

AS control of IP networks changes frequently, therefore it can be useful to get
the changes between two ASmaps via the `diff` and `diff_addrs` commands.

`diff` takes two ASmap files, and returns a file detailing the changes
in the state of a network's AS assignment between the first and the second file.
This command may take a few minutes to run, depending on your machine.

The example below shows the three possible output states:
- reassigned to a new AS (`AS26496 # was AS20738`),
- present in the first but not the second (`# 220.157.65.0/24 was AS9723`),
- or present in the second but not the first (`# was unassigned`).

```
217.199.160.0/19 AS26496 # was AS20738
# 220.157.65.0/24 was AS9723
216.151.172.0/23 AS400080 # was unassigned
2001:470:49::/48 AS20205 # was AS6939
# 2001:678:bd0::/48 was AS207631
2001:67c:308::/48 AS26496 # was unassigned
```
`diff` accepts a `--ignore-unassigned`/`-i` flag
which ignores networks present in the second but not the first.

`diff_addrs` is intended to provide changes between two ASmaps and
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
and pass in the address file as the third argument:
```
python3 asmap-tool.py diff_addrs path/to/first.file path/to/second.file addrs.json
```
