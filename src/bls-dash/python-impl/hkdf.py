import hashlib
import hmac
from math import ceil

BLOCK_SIZE = 32


def extract(salt: bytes, ikm: bytes) -> bytes:
    h = hmac.new(salt, ikm, hashlib.sha256)
    return h.digest()


def expand(L: int, prk: bytes, info: bytes) -> bytes:
    N: int = ceil(L / BLOCK_SIZE)
    bytes_written: int = 0
    okm: bytes = b""

    for i in range(1, N + 1):
        if i == 1:
            h = hmac.new(prk, info + bytes([1]), hashlib.sha256)
            T: bytes = h.digest()
        else:
            h = hmac.new(prk, T + info + bytes([i]), hashlib.sha256)
            T = h.digest()
        to_write = L - bytes_written
        if to_write > BLOCK_SIZE:
            to_write = BLOCK_SIZE
        okm += T[:to_write]
        bytes_written += to_write
    assert bytes_written == L
    return okm


def extract_expand(L: int, key: bytes, salt: bytes, info: bytes) -> bytes:
    prk = extract(salt, key)
    return expand(L, prk, info)


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
