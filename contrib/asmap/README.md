# ASMap Tool

Tool for performing various operations on textual and binary asmap files,
particularly encoding/compressing the raw data to the binary format that can
be used in Bitcoin Core with the `-asmap` option.

## Example usage

To decode/encode asmap files use the `decode` and `encode` commands.

```
python3 asmap-tool.py encode /path/to/input.file /path/to/output.file
python3 asmap-tool.py decode /path/to/input.file /path/to/output.file
```

The `diff` command computes the differences between two asmap files. They can
be text or binary.

```
python3 asmap-tool.py diff /path/to/first.file /path/to/second.file
```

The `diff_addrs` command computes the differences between two asmap files but
limited to a set of addresses that need to be passed as a third argument. This
should be a list of known addresses from `getnodeaddresses`.

```
bitcoin-cli getnodeaddresses 0 > addrs.json
python3 asmap-tool.py diff /path/to/first.file /path/to/second.file addrs.json
```
