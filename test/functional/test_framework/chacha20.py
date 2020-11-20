# Copyright (c) 2016-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Native Python ChaCha20 implementation with a 64-bit nonce."""


class ChaCha20:
    """Class representing a ChaCha20 cipher instance."""

    def __init__(self, key_bytes, nonce_int):
        # split 256-bit key into list of 8 32-bit integers
        self.key = self.__le_bytes_to_int32_words(key_bytes, 8)
        # split 64-bit nonce into list of 2 32-bit integers
        nonce_bytes = nonce_int.to_bytes(8, 'little')
        self.nonce = self.__le_bytes_to_int32_words(nonce_bytes, 2)

    def encrypt(self, plaintext, counter_init=0):
        """Encrypt the plaintext"""
        byte_length = len(plaintext)
        stream = self.keystream(byte_length, counter_init)
        ciphertext = [plaintext[i] ^ stream[i] for i in range(byte_length)]

        return bytes(ciphertext)

    def keystream(self, length, counter_init=0):
        """Returns the keystream for a given length"""
        blocks = -(length // -64)  # ceiling division
        out = b''

        for i in range(blocks):
            counter_bytes = (counter_init + i).to_bytes(8, 'little')
            counter = self.__le_bytes_to_int32_words(counter_bytes, 2)
            out += self.__serialize(self.__block(counter))

        return out[:length]

    def decrypt(self, ciphertext, counter=1):
        """Decrypt the ciphertext"""
        return self.encrypt(ciphertext, counter)

    def __block(self, counter):
        # See RFC 8439 section 2.3 for chacha20 parameters
        BLOCK_CONSTANTS = [0x61707865, 0x3320646e, 0x79622d32, 0x6b206574]
        init = BLOCK_CONSTANTS + self.key + counter + self.nonce
        s = init.copy()

        for _ in range(10):
            self.__doubleround(s)
        for i in range(16):
            s[i] = (s[i] + init[i]) & 0xffffffff

        return s

    @staticmethod
    def __serialize(block):
        return b''.join([(word).to_bytes(4, 'little') for word in block])

    @staticmethod
    def __le_bytes_to_int32_words(le_bytes, length):
        return [
            int.from_bytes(
                le_bytes[i * 4:i * 4 + 4], byteorder='little'
            ) for i in range(length)
        ]

    @staticmethod
    def __doubleround(s):
        """Apply a ChaCha20 double round to 16-element state array s.

        See https://cr.yp.to/chacha/chacha-20080128.pdf and https://tools.ietf.org/html/rfc8439
        """
        QUARTER_ROUNDS = [
            # columns
            (0, 4, 8, 12),
            (1, 5, 9, 13),
            (2, 6, 10, 14),
            (3, 7, 11, 15),
            # diagonals
            (0, 5, 10, 15),
            (1, 6, 11, 12),
            (2, 7, 8, 13),
            (3, 4, 9, 14)
        ]

        for a, b, c, d in QUARTER_ROUNDS:
            s[a] = (s[a] + s[b]) & 0xffffffff
            s[d] = ChaCha20.__rot32(s[d] ^ s[a], 16)
            s[c] = (s[c] + s[d]) & 0xffffffff
            s[b] = ChaCha20.__rot32(s[b] ^ s[c], 12)
            s[a] = (s[a] + s[b]) & 0xffffffff
            s[d] = ChaCha20.__rot32(s[d] ^ s[a], 8)
            s[c] = (s[c] + s[d]) & 0xffffffff
            s[b] = ChaCha20.__rot32(s[b] ^ s[c], 7)

    @staticmethod
    def __rot32(v, bits):
        """Rotate the 32-bit value v left by bits bits."""
        bits %= 32  # Make sure the term below does not throw an exception
        return ((v << bits) & 0xffffffff) | (v >> (32 - bits))
