#!/usr/bin/env python3
# Copyright (c) 2013-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Generate seeds.txt from Pieter's DNS seeder
#

import argparse
import ipaddress
import re
import sys
import collections
from typing import List, Dict, Union

from asmap import ASMap, net_to_prefix

NSEEDS=512

MAX_SEEDS_PER_ASN = {
    'ipv4': 2,
    'ipv6': 10,
}

MIN_BLOCKS = 730000

# These are hosts that have been observed to be behaving strangely (e.g.
# aggressively connecting to every node).
with open("suspicious_hosts.txt", mode="r", encoding="utf-8") as f:
    SUSPICIOUS_HOSTS = {s.strip() for s in f if s.strip()}


PATTERN_IPV4 = re.compile(r"^((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})):(\d+)$")
PATTERN_IPV6 = re.compile(r"^\[([0-9a-z:]+)\]:(\d+)$")
PATTERN_ONION = re.compile(r"^([abcdefghijklmnopqrstuvwxyz234567]{16}\.onion):(\d+)$")
PATTERN_AGENT = re.compile(
    r"^/Satoshi:("
    r"4.1.(0|1|2|3)|"
    r"4.2.(0|1)|"
    r"4.3.(0|1)|"
    r")")

def parseline(line: str) -> Union[dict, None]:
    """ Parses a line from `seeds_main.txt` into a dictionary of details for that line.
    or `None`, if the line could not be parsed.
    """
    sline = line.split()
    if len(sline) < 11:
        # line too short to be valid, skip it.
        return None
    m = PATTERN_IPV4.match(sline[0])
    sortkey = None
    ip = None
    if m is None:
        m = PATTERN_IPV6.match(sline[0])
        if m is None:
            m = PATTERN_ONION.match(sline[0])
            if m is None:
                return None
            else:
                net = 'onion'
                ipstr = sortkey = m.group(1)
                port = int(m.group(2))
        else:
            net = 'ipv6'
            if m.group(1) in ['::']: # Not interested in localhost
                return None
            ipstr = m.group(1)
            sortkey = ipstr # XXX parse IPv6 into number, could use name_to_ipv6 from generate-seeds
            port = int(m.group(2))
    else:
        # Do IPv4 sanity check
        ip = 0
        for i in range(0,4):
            if int(m.group(i+2)) < 0 or int(m.group(i+2)) > 255:
                return None
            ip = ip + (int(m.group(i+2)) << (8*(3-i)))
        if ip == 0:
            return None
        net = 'ipv4'
        sortkey = ip
        ipstr = m.group(1)
        port = int(m.group(6))
    # Skip bad results.
    if sline[1] == 0:
        return None
    # Extract uptime %.
    uptime30 = float(sline[7][:-1])
    # Extract Unix timestamp of last success.
    lastsuccess = int(sline[2])
    # Extract protocol version.
    version = int(sline[10])
    # Extract user agent.
    agent = sline[11][1:-1]
    # Extract service flags.
    service = int(sline[9], 16)
    # Extract blocks.
    blocks = int(sline[8])
    # Construct result.
    return {
        'net': net,
        'ip': ipstr,
        'port': port,
        'ipnum': ip,
        'uptime': uptime30,
        'lastsuccess': lastsuccess,
        'version': version,
        'agent': agent,
        'service': service,
        'blocks': blocks,
        'sortkey': sortkey,
    }

def dedup(ips: List[Dict]) -> List[Dict]:
    """ Remove duplicates from `ips` where multiple ips share address and port. """
    d = {}
    for ip in ips:
        d[ip['ip'],ip['port']] = ip
    return list(d.values())

def filtermultiport(ips: List[Dict]) -> List[Dict]:
    """ Filter out hosts with more nodes per IP"""
    hist = collections.defaultdict(list)
    for ip in ips:
        hist[ip['sortkey']].append(ip)
    return [value[0] for (key,value) in list(hist.items()) if len(value)==1]

# Based on Greg Maxwell's seed_filter.py
def filterbyasn(asmap: ASMap, ips: List[Dict], max_per_asn: Dict, max_per_net: int) -> List[Dict]:
    """ Prunes `ips` by
    (a) trimming ips to have at most `max_per_net` ips from each net (e.g. ipv4, ipv6); and
    (b) trimming ips to have at most `max_per_asn` ips from each asn in each net.
    """
    # Sift out ips by type
    ips_ipv46 = [ip for ip in ips if ip['net'] in ['ipv4', 'ipv6']]
    ips_onion = [ip for ip in ips if ip['net'] == 'onion']

    # Filter IPv46 by ASN, and limit to max_per_net per network
    result = []
    net_count: Dict[str, int] = collections.defaultdict(int)
    asn_count: Dict[int, int] = collections.defaultdict(int)

    for i, ip in enumerate(ips_ipv46):
        if net_count[ip['net']] == max_per_net:
            # do not add this ip as we already too many
            # ips from this network
            continue
        asn = asmap.lookup(net_to_prefix(ipaddress.ip_network(ip['ip'])))
        if not asn or asn_count[ip['net'], asn] == max_per_asn[ip['net']]:
            # do not add this ip as we already have too many
            # ips from this ASN on this network
            continue
        asn_count[ip['net'], asn] += 1
        net_count[ip['net']] += 1
        ip['asn'] = asn
        result.append(ip)

    # Add back Onions (up to max_per_net)
    result.extend(ips_onion[0:max_per_net])
    return result

def ip_stats(ips: List[Dict]) -> str:
    """ Format and return pretty string from `ips`. """
    hist: Dict[str, int] = collections.defaultdict(int)
    for ip in ips:
        if ip is not None:
            hist[ip['net']] += 1

    return f"{hist['ipv4']:6d} {hist['ipv6']:6d} {hist['onion']:6d}"

def parse_args():
    argparser = argparse.ArgumentParser(description='Generate a list of syscoin node seed ip addresses.')
    argparser.add_argument("-a","--asmap", help='the location of the asmap asn database file (required)', required=True)
    argparser.add_argument("-s","--seeds", help='the location of the DNS seeds file (required)', required=True)
    return argparser.parse_args()

def main():
    args = parse_args()

    print(f'Loading asmap database "{args.asmap}"…', end='', file=sys.stderr, flush=True)
    with open(args.asmap, 'rb') as f:
        asmap = ASMap.from_binary(f.read())
    print('Done.', file=sys.stderr)

    print('Loading and parsing DNS seeds…', end='', file=sys.stderr, flush=True)
    with open(args.seeds, 'r', encoding='utf8') as f:
        lines = f.readlines()
    ips = [parseline(line) for line in lines]
    print('Done.', file=sys.stderr)

    print('\x1b[7m  IPv4   IPv6  Onion Pass                                               \x1b[0m', file=sys.stderr)
    print(f'{ip_stats(ips):s} Initial', file=sys.stderr)
    # Skip entries with invalid address.
    ips = [ip for ip in ips if ip is not None]
    print(f'{ip_stats(ips):s} Skip entries with invalid address', file=sys.stderr)
    # Skip duplicates (in case multiple seeds files were concatenated)
    ips = dedup(ips)
    print(f'{ip_stats(ips):s} After removing duplicates', file=sys.stderr)
    # Skip entries from suspicious hosts.
    ips = [ip for ip in ips if ip['ip'] not in SUSPICIOUS_HOSTS]
    print(f'{ip_stats(ips):s} Skip entries from suspicious hosts', file=sys.stderr)
    # Enforce minimal number of blocks.
    ips = [ip for ip in ips if ip['blocks'] >= MIN_BLOCKS]
    print(f'{ip_stats(ips):s} Enforce minimal number of blocks', file=sys.stderr)
    # Require service bit 1.
    ips = [ip for ip in ips if (ip['service'] & 1) == 1]
    print(f'{ip_stats(ips):s} Require service bit 1', file=sys.stderr)
    # Require at least 50% 30-day uptime for clearnet, 10% for onion.
    req_uptime = {
        'ipv4': 50,
        'ipv6': 50,
        'onion': 10,
    }
    ips = [ip for ip in ips if ip['uptime'] > req_uptime[ip['net']]]
    print(f'{ip_stats(ips):s} Require minimum uptime', file=sys.stderr)
    # Require a known and recent user agent.
    ips = [ip for ip in ips if PATTERN_AGENT.match(ip['agent'])]
    print(f'{ip_stats(ips):s} Require a known and recent user agent', file=sys.stderr)
    # Sort by availability (and use last success as tie breaker)
    ips.sort(key=lambda x: (x['uptime'], x['lastsuccess'], x['ip']), reverse=True)
    # Filter out hosts with multiple syscoin ports, these are likely abusive
    ips = filtermultiport(ips)
    print(f'{ip_stats(ips):s} Filter out hosts with multiple syscoin ports', file=sys.stderr)
    # Look up ASNs and limit results, both per ASN and globally.
    ips = filterbyasn(asmap, ips, MAX_SEEDS_PER_ASN, NSEEDS)
    print(f'{ip_stats(ips):s} Look up ASNs and limit results per ASN and per net', file=sys.stderr)
    # Sort the results by IP address (for deterministic output).
    ips.sort(key=lambda x: (x['net'], x['sortkey']))
    for ip in ips:
        if ip['net'] == 'ipv6':
            print(f"[{ip['ip']}]:{ip['port']}", end="")
        else:
            print(f"{ip['ip']}:{ip['port']}", end="")
        if 'asn' in ip:
            print(f" # AS{ip['asn']}", end="")
        print()

if __name__ == '__main__':
    main()
