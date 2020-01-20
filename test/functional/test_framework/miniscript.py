#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Classes and methods to encode and decode miniscripts"""

from enum import Enum


class Property:
    """Miniscript expression property"""
    # "B": Base type
    # "V": Verify type
    # "K": Key type
    # "W": Wrapped type
    # "z": Zero-arg property
    # "o": One-arg property
    # "n": Nonzero arg property
    # "d": Dissatisfiable property
    # "u": Unit property
    # "e": Expression property
    # "f": Forced property
    # "s": Safe property
    # "m": Nonmalleable property
    # "x": Expensive verify
    types = "BVKW"
    props = "zonduefsmx"

    def __init__(self):
        for literal in self.types+self.props:
            setattr(self, literal, False)

    def from_string(self, property_str):
        """Construct property from string of valid property and types"""
        for char in property_str:
            assert(hasattr(self, char))
            setattr(self, char, True)
        assert self.is_valid()
        return self

    def to_string(self):
        """Generate string representation of property"""
        property_str = ""
        for char in self.types+self.props:
            if getattr(self, char):
                property_str += char
        return property_str

    def is_valid(self):
        # Can only be of a single type.
        num_types = 0
        for typ in self.types:
            if getattr(self, typ):
                num_types += 1
        assert num_types == 1

        # Check for conflicts in type & properties.
        return \
            (not self.z or not self.o) and \
            (not self.n or not self.z) and \
            (not self.V or not self.d) and \
            (not self.K or self.u) and \
            (not self.V or not self.u) and \
            (not self.e or not self.f) and \
            (not self.e or self.d) and \
            (not self.V or not self.e) and \
            (not self.d or not self.f) and \
            (not self.V or self.f) and \
            (not self.K or self.s) and \
            (not self.z or self.m)


class NodeType(Enum):
    JUST_0 = 0
    JUST_1 = 1
    PK = 2
    PK_H = 3
    OLDER = 4
    AFTER = 5
    SHA256 = 6
    HASH256 = 7
    RIPEMD160 = 8
    HASH160 = 9
    WRAP_A = 10
    WRAP_S = 11
    WRAP_C = 12
    WRAP_T = 13
    WRAP_D = 14
    WRAP_V = 15
    WRAP_J = 16
    WRAP_N = 17
    WRAP_U = 18
    WRAP_L = 19
    AND_V = 20
    AND_B = 21
    AND_N = 22
    OR_B = 23
    OR_C = 24
    OR_D = 25
    OR_I = 26
    ANDOR = 27
    THRESH = 28
    THRESH_M = 29
