
# UniValue

## Summary

A universal value class, with JSON encoding and decoding.

UniValue is an abstract data type that may be a null, boolean, string,
number, array container, or a key/value dictionary container, nested to
an arbitrary depth.

This class is aligned with the JSON standard, [RFC
7159](https://tools.ietf.org/html/rfc7159.html).

## Library usage

This is a fork of univalue used by XBit Core. It is not maintained for usage
by other projects. Notably, the API may break in non-backward-compatible ways.

Other projects looking for a maintained library should use the upstream
univalue at https://github.com/jgarzik/univalue.
