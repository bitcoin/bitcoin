import hashlib

HMAC_BLOCK_SIZE = 64


def hash256(m):
    if type(m) != bytes:
        m = m.encode("utf-8")
    return hashlib.sha256(m).digest()


def hash512(m):
    if type(m) != bytes:
        m = m.encode("utf-8")
    return hash256(m + bytes([0])) + hash256(m + bytes([1]))


def hmac256(m, k):
    if type(m) != bytes and type(m) != bytearray:
        m = m.encode("utf-8")
    if type(k) != bytes and type(k) != bytearray:
        k = k.encode("utf-8")
    k = bytes(k)
    if len(k) > HMAC_BLOCK_SIZE:
        k = hash256(k)
    while len(k) < HMAC_BLOCK_SIZE:
        k += bytes([0])
    opad = bytes([0x5C] * HMAC_BLOCK_SIZE)
    ipad = bytes([0x36] * HMAC_BLOCK_SIZE)
    kopad = bytes([k[i] ^ opad[i] for i in range(HMAC_BLOCK_SIZE)])
    kipad = bytes([k[i] ^ ipad[i] for i in range(HMAC_BLOCK_SIZE)])
    return hash256(kopad + hash256(kipad + m))


"""
Copyright 2020 Chia Network Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
