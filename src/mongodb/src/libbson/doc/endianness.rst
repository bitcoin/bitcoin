:man_page: bson_endianness

Endianness
==========

The BSON specification dictates that the encoding format is in little-endian. Many implementations simply ignore endianness altogether and expect that they are to be run on little-endian. Libbson supports both Big and Little Endian systems. This means we use ``memcpy()`` when appropriate instead of dereferencing and properly convert to and from the hoste endian format. We expect the compiler intrinsics to optimize it to a dereference when possible.

