#!/usr/bin/env python
#
# Generate seeds.txt from Pieter's DNS seeder
#

import collections
import dns.resolver
import logging
import re
import socket
import struct
import sys
from itertools import islice
from UserDict import UserDict


MAX_ENTRIES = 512
MAX_ENTRIES_PER_ASN = 2
MIN_BLOCKS = 337600

PATTERN_IPV4 = re.compile(
        r"^((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})):(\d+)$")
PATTERN_IPV6 = re.compile(r"^\[([0-9a-z:]+)\]:(\d+)$")
PATTERN_ONION = re.compile(
        r"^([abcdefghijklmnopqrstuvwxyz234567]{16}\.onion):(\d+)$")
PATTERN_AGENT = re.compile(
        r"^(\/Satoshi:0\.8\.6\/|"
        r"\/Satoshi:0\.9\.(2|3|4|5)\/|"
        r"\/Satoshi:0\.10\.\d{1,2}\/|"
        r"\/Satoshi:0\.11\.\d{1,2}\/)$")


# These are hosts that have been observed to be behaving strangely (e.g.
# aggressively connecting to every node).
with open('suspicious_hosts.txt') as f:
    SUSPICIOUS_HOSTS = set(f.readlines())


class Address(object):

    PATTERNS = {'ipv4': PATTERN_IPV4,
                'ipv6': PATTERN_IPV6,
                'onion': PATTERN_ONION}

    INET_FORMATS = {'ipv4': (socket.AF_INET, '!L'),
                    'ipv6': (socket.AF_INET6, '!QQ')}

    def __init__(self, host, port, address_type, sortable_host):
        self.host = host
        self.port = port
        self.address_type = address_type
        self.sortable_host = sortable_host

    @property
    def asn(self):
        if hasattr(self, '_asn'):
            return self._asn

    def get_asn(self):
        """Discover the ASN (Autonomous System Number) for this entry.

        This also caches the value to avoid future DNS requests."""
        if self.address_type == 'onion':
            return None

        if self.address_type == 'ipv4':
            reversed_ip = '.'.join(reversed(self.host.split('.')))
            domain = reversed_ip + '.origin.asn.cymru.com'
            try:
                answer = dns.resolver.query(domain, 'TXT').response.answer[0][0]
                self._asn = int(answer.strings[0].split('|')[0])
            except:
                return None

            return self._asn

        return None

    @classmethod
    def get_address_type(cls, address_string):
        """Returns ipv4, ipv6, onion, or None for a given address."""
        for address_type, pattern in cls.PATTERNS.items():
            if pattern.match(address_string):
                return address_type
        return None

    @classmethod
    def from_combined_string(cls, address_string):
        """Given an address, return a parsed Address object."""

        address_type = cls.get_address_type(address_string)
        if not address_type:
            raise ValueError('Invalid address ("%s")' % address_string)

        host, port = address_string.rsplit(':', 1)
        if address_type == 'ipv6':
            host = host[1:-1]

        if not host or not port:
            raise ValueError('Invalid address ("%s")' % address_string)

        sortable_host = host

        if address_type in cls.INET_FORMATS:
            type_id, struct_format = cls.INET_FORMATS[address_type]
            try:
                packed_ip = socket.inet_pton(type_id, host)
                sortable_host = struct.unpack(struct_format, packed_ip)[0]
            except:
                raise ValueError('Invalid %s host ("%s")' %
                                 (address_type, host))

        return cls(host, int(port), address_type, sortable_host)

    def __repr__(self):
        return str({"host": self.host, "port": self.port, "asn": self.asn,
                    "address_type": self.address_type,
                    "sortable_host": self.sortable_host})

    def __str__(self):
        pattern = u'%s:%i'
        if self.address_type == 'ipv6':
            pattern = u'[%s];%i'
        return pattern % (self.host, self.port)

    __unicode__ = __str__


class Entry(UserDict):
    """Represents a single line in the seeds.txt file."""

    _percent = lambda _: float(_[:-1])
    SCHEMA = [
            ('address', Address.from_combined_string),
            ('good', bool),
            ('lastSuccess', int),
            ('uptime2h', _percent),
            ('uptime8h', _percent),
            ('uptime1d', _percent),
            ('uptime7d', _percent),
            ('uptime30d', _percent),
            ('blocks', int),
            ('services', lambda _: int(_, 16)),
            ('version', int),
            ('agent', lambda _: _[1:-1])]

    @classmethod
    def from_raw_line(cls, line):
        """Given a line from the seeds.txt file, return an Entry.

        If the line is invalid, return None.
        """
        if not line or line.startswith('#'):
            return None

        pieces = line.split()
        if len(pieces) < 11:
            logging.warning('Not enough fields (only %d): %s' %
                            (len(pieces), line))
            return None

        entry = cls()
        for (key, cast), value in zip(cls.SCHEMA, pieces):
            try:
                entry[key] = cast(value)
            except:
                logging.error('Invalid value for %s ("%s")' % (key, value))
                return None
        return entry

    @property
    def address(self):
        return self['address']

    def is_valid(self):
        """Decides whether a given entry is valid."""
        # Skip over any hosts that are telling us they're not good.
        if not self['good']: return False

        # Skip over any suspicious hosts.
        if self['address'].host in SUSPICIOUS_HOSTS: return False

        # Skip over hosts that don't have enough blocks.
        if self['blocks'] < MIN_BLOCKS: return False

        # Skip over hosts that don't have service bit 1.
        if not self['services'] & 1: return False

        # Skip over hosts that don't have 50% 30d uptime.
        if self['uptime30d'] < 50: return False

        # Skip over hosts with unknown agents.
        if not PATTERN_AGENT.match(self['agent']): return False

        return True

    @property
    def sort_key(self):
        """Returns a sortable tuple, in thie case using uptime and success."""
        return (self['uptime30d'], self['lastSuccess'])

    def __hash__(self):
        return self.address.sortable_host

    def __eq__(self, other):
        """For purposes of uniqueness, entries are equal based on address."""
        return self.address.sortable_host == other.address.sortable_host


def get_entries_limited_by_asn(entries, max_per_asn=MAX_ENTRIES_PER_ASN):
    """Return entries limited to a certain number per ASN."""
    asn_counter = {}
    for entry in entries:
        asn = entry.address.get_asn()
        if not asn:
            yield entry
            continue

        count = asn_counter.get(asn, 0) + 1
        if count > max_per_asn:
            continue

        # Update the counter
        asn_counter[asn] = count
        yield entry


def main():
    # Use stdin as the input.
    lines = sys.stdin

    # Parse all the lines and ignore None values.
    entries = filter(None, map(Entry.from_raw_line, lines))

    # Only include entries we consider to be valid.
    entries = filter(lambda e: e.is_valid(), entries)

    # Sort the entries using their sort key (uptime, last success)
    entries = sorted(entries, key=lambda e: e.sort_key,
                     reverse=True)

    # Limit the number of entries per ASN.
    entries = get_entries_limited_by_asn(entries)

    # Iterate through and print out a limited number of entries.
    for entry in islice(entries, MAX_ENTRIES):
        print entry.address


if __name__ == '__main__':
    main()
