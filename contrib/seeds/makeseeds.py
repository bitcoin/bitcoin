#!/usr/bin/env python3
# Copyright (c) 2013-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Generate seeds.txt from "protx list valid 1"
# then create onion_seeds.txt and add some active onion services to it; check tor.md for some
#

import re
import sys
import dns.resolver
import collections
import json
import multiprocessing
from typing import List, Dict, Union

NSEEDS=512

MAX_SEEDS_PER_ASN=4

MAX_SEEDS_PER_ASN = {
    'ipv4': 4,
    'ipv6': 10,
}

MIN_BLOCKS = 2_300_000

PATTERN_IPV4 = re.compile(r"^((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})):(\d+)$")
PATTERN_IPV6 = re.compile(r"^\[([0-9a-z:]+)\]:(\d+)$")
PATTERN_ONION = re.compile(r"^([a-z2-7]{56}\.onion):(\d+)$")


def parseip(ip_in: str) -> Union[dict, None]:
    """ Parses a line from `seeds_main.txt` into a dictionary of details for that line.
    or `None`, if the line could not be parsed.
    """
    m = PATTERN_IPV4.match(ip_in)
    ip = None
    if m is None:
        m = PATTERN_IPV6.match(ip_in)
        if m is None:
            m = PATTERN_ONION.match(ip_in)
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

    return {
        "net": net,
        "ip": ipstr,
        "port": port,
        "ipnum": ip,
        "sortkey": sortkey
    }

def filtermulticollateralhash(mns : List[Dict]) -> List[Dict]:
    '''Filter out MNs sharing the same collateral hash'''
    hist = collections.defaultdict(list)
    for mn in mns:
        hist[mn['collateralHash']].append(mn)
    return [mn for mn in mns if len(hist[mn['collateralHash']]) == 1]

def filtermulticollateraladdress(mns : List[Dict]) -> List[Dict]:
    '''Filter out MNs sharing the same collateral address'''
    hist = collections.defaultdict(list)
    for mn in mns:
        hist[mn['collateralAddress']].append(mn)
    return [mn for mn in mns if len(hist[mn['collateralAddress']]) == 1]

def filtermultipayoutaddress(mns : List[Dict]) -> List[Dict]:
    '''Filter out MNs sharing the same payout address'''
    hist = collections.defaultdict(list)
    for mn in mns:
        hist[mn['state']['payoutAddress']].append(mn)
    return [mn for mn in mns if len(hist[mn['state']['payoutAddress']]) == 1]

def resolveasn(resolver, ip : Dict) -> Union[int, None]:
    """ Look up the asn for an `ip` address by querying cymru.com
    on network `net` (e.g. ipv4 or ipv6).

    Returns in integer ASN or None if it could not be found.
    """
    try:
        if ip['net'] == 'ipv4':
            ipaddr = ip['ip']
            prefix = '.origin'
        else:                  # http://www.team-cymru.com/IP-ASN-mapping.html
            res = str()                         # 2001:4860:b002:23::68
            for nb in ip['ip'].split(':')[:4]:  # pick the first 4 nibbles
                for c in nb.zfill(4):           # right padded with '0'
                    res += c + '.'              # 2001 4860 b002 0023
            ipaddr = res.rstrip('.')            # 2.0.0.1.4.8.6.0.b.0.0.2.0.0.2.3
            prefix = '.origin6'

        asn = int([x.to_text() for x in resolver.resolve('.'.join(
                   reversed(ipaddr.split('.'))) + prefix + '.asn.cymru.com',
                   'TXT').response.answer][0].split('\"')[1].split(' ')[0])
        return asn
    except Exception:
        sys.stderr.write('ERR: Could not resolve ASN for "' + ip + '"\n')
        return None

# Based on Greg Maxwell's seed_filter.py
def filterbyasn(ips: List[Dict], max_per_asn: Dict, max_per_net: int) -> List[Dict]:
    """ Prunes `ips` by
    (a) trimming ips to have at most `max_per_net` ips from each net (e.g. ipv4, ipv6); and
    (b) trimming ips to have at most `max_per_asn` ips from each asn in each net.
    """
    # Sift out ips by type
    ips_ipv46 = [ip for ip in ips if ip['net'] in ['ipv4', 'ipv6']]
    ips_onion = [ip for ip in ips if ip['net'] == 'onion']

    my_resolver = dns.resolver.Resolver()
    pool = multiprocessing.Pool(processes=16)

    # OpenDNS servers
    my_resolver.nameservers = ['208.67.222.222', '208.67.220.220']

    # Resolve ASNs in parallel
    asns = [pool.apply_async(resolveasn, args=(my_resolver, ip)) for ip in ips_ipv46]

    # Filter IPv46 by ASN, and limit to max_per_net per network
    result = []
    net_count: Dict[str, int] = collections.defaultdict(int)
    asn_count: Dict[int, int] = collections.defaultdict(int)

    for i, ip in enumerate(ips_ipv46):
        if i % 10 == 0:
            # give progress update
            print(f"{i:6d}/{len(ips_ipv46)} [{100*i/len(ips_ipv46):04.1f}%]\r", file=sys.stderr, end='', flush=True)

        if net_count[ip['net']] == max_per_net:
            # do not add this ip as we already too many
            # ips from this network
            continue
        asn = asns[i].get()
        if asn is None or asn_count[asn] == max_per_asn[ip['net']]:
            # do not add this ip as we already have too many
            # ips from this ASN on this network
            continue
        asn_count[asn] += 1
        net_count[ip['net']] += 1
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

def main():
    # This expects a json as outputted by "protx list valid 1"
    if len(sys.argv) > 1:
        with open(sys.argv[1], 'r', encoding="utf8") as f:
            mns = json.load(f)
    else:
        mns = json.load(sys.stdin)

    onions = []
    if len(sys.argv) > 2:
        with open(sys.argv[2], 'r', encoding="utf8") as f:
            onions = f.read().split('\n')

    print(f'Total mns: {len(mns)}', file=sys.stderr)
    # Skip PoSe banned MNs
    mns = [mn for mn in mns if mn['state']['PoSeBanHeight'] == -1]
    print(f'After skip entries from PoSe banned MNs: {len(mns)}', file=sys.stderr)
    # Skip MNs with < 10000 confirmations
    mns = [mn for mn in mns if mn['confirmations'] >= 10000]
    print(f'After skip MNs with less than 10000 confirmations: {len(mns)}', file=sys.stderr)

    # Filter out MNs which are definitely from the same person/operator
    mns = filtermulticollateralhash(mns)
    mns = filtermulticollateraladdress(mns)
    mns = filtermultipayoutaddress(mns)
    print(f'After removing duplicates: {len(mns)}', file=sys.stderr)

    # Extract IPs
    ips = [parseip(mn['state']['addresses'][0]) for mn in mns]
    for onion in onions:
        parsed = parseip(onion)
        if parsed is not None:
            ips.append(parsed)

    print('\x1b[7m  IPv4   IPv6  Onion Pass                                               \x1b[0m', file=sys.stderr)
    print(f'{ip_stats(ips):s} Initial', file=sys.stderr)
    # Skip entries with invalid address.
    ips = [ip for ip in ips if ip is not None]
    print(f'{ip_stats(ips):s} Skip entries with invalid address', file=sys.stderr)

    # Look up ASNs and limit results, both per ASN and globally.
    ips = filterbyasn(ips, MAX_SEEDS_PER_ASN, NSEEDS)
    print(f'{ip_stats(ips):s} Look up ASNs and limit results per ASN and per net', file=sys.stderr)
    # Sort the results by IP address (for deterministic output).
    ips.sort(key=lambda x: (x['net'], x['sortkey']), reverse=True)

    for ip in ips:
        if ip['net'] == 'ipv6':
            print('[%s]:%i' % (ip['ip'], ip['port']))
        else:
            print('%s:%i' % (ip['ip'], ip['port']))

if __name__ == '__main__':
    main()
