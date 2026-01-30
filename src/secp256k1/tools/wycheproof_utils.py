# Copyright (c) 2024 Random "Randy" Lattice and Sean Andersen
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.
'''
Utility functions for generating C files for testvectors from the Wycheproof project.
'''

def to_c_array(x):
    if x == "":
        return ""
    s = ',0x'.join(a + b for a, b in zip(x[::2], x[1::2]))
    return "0x" + s
