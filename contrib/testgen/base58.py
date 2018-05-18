'''
Bitcoin base58 encoding and decoding.

Based on https://bitcointalk.org/index.php?topic=1026.0 (public domain)
'''
import hashlib

# for compatibility with following code...
class SHA256:
    new = hashlib.sha256

if str != bytes:
    # Python 3.x
    def ord(c):
        return c
    def chr(n):
        return bytes( (n,) )

__b58chars = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'
__b58base = len(__b58chars)
b58chars = __b58chars

def b58encode(v):
    """ encode v, which is a string of bytes, to base58.
    """
    long_value = 0
    for (i, c) in enumerate(v[::-1]):
        long_value += (256**i) * ord(c)

    result = ''
    while long_value >= __b58base:
        div, mod = divmod(long_value, __b58base)
        result = __b58chars[mod] + result
        long_value = div
    result = __b58chars[long_value] + result

    # Bitcoin does a little leading-zero-compression:
    # leading 0-bytes in the input become leading-1s
    nPad = 0
    for c in v:
        if c == '\0': nPad += 1
        else: break

    return (__b58chars[0]*nPad) + result

def b58decode(v, length = None):
    """ decode v into a string of len bytes
    """
    long_value = 0
    for (i, c) in enumerate(v[::-1]):
        long_value += __b58chars.find(c) * (__b58base**i)

    result = bytes()
    while long_value >= 256:
        div, mod = divmod(long_value, 256)
        result = chr(mod) + result
        long_value = div
    result = chr(long_value) + result

    nPad = 0
    for c in v:
        if c == __b58chars[0]: nPad += 1
        else: break

    result = chr(0)*nPad + result
    if length is not None and len(result) != length:
        return None

    return result

def checksum(v):
    """Return 32-bit checksum based on SHA256"""
    return SHA256.new(SHA256.new(v).digest()).digest()[0:4]

def b58encode_chk(v):
    """b58encode a string, with 32-bit checksum"""
    return b58encode(v + checksum(v))

def b58decode_chk(v):
    """decode a base58 string, check and remove checksum"""
    result = b58decode(v)
    if result is None:
        return None
    h3 = checksum(result[:-4])
    if result[-4:] == checksum(result[:-4]):
        return result[:-4]
    else:
        return None

def get_bcaddress_version(strAddress):
    """ Returns None if strAddress is invalid.  Otherwise returns integer version of address. """
    addr = b58decode_chk(strAddress)
    if addr is None or len(addr)!=21: return None
    version = addr[0]
    return ord(version)

if __name__ == '__main__':
    # Test case (from http://gitorious.org/bitcoin/python-base58.git)
    assert get_bcaddress_version('15VjRaDX9zpbA8LVnbrCAFzrVzN7ixHNsC') is 0
    _ohai = 'o hai'.encode('ascii')
    _tmp = b58encode(_ohai)
    assert _tmp == 'DYB3oMS'
    assert b58decode(_tmp, 5) == _ohai
    print("Tests passed")
