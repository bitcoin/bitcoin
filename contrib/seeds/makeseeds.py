#!/usr/bin/env python3
# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Generate seeds.txt from "protx list valid 1"
#

import re
import sys
import dns.resolver
import collections
import json
import multiprocessing

NSEEDS=512

MAX_SEEDS_PER_ASN=4

# These are hosts that have been observed to be behaving strangely (e.g.
# aggressively connecting to every node).
SUSPICIOUS_HOSTS = {
}

PATTERN_IPV4 = re.compile(r"^((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})):(\d+)$")
PATTERN_IPV6 = re.compile(r"^\[([0-9a-z:]+)\]:(\d+)$")
PATTERN_ONION = re.compile(r"^([abcdefghijklmnopqrstuvwxyz234567]{16}\.onion):(\d+)$")

def parseip(ip):
    m = PATTERN_IPV4.match(ip)
    sortkey = None
    ip = None
    if m is None:
        m = PATTERN_IPV6.match(ip)
        if m is None:
            m = PATTERN_ONION.match(ip)
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

def filtermulticollateralhash(mns):
    '''Filter out MNs sharing the same collateral hash'''
    hist = collections.defaultdict(list)
    for mn in mns:
        hist[mn['collateralHash']].append(mn)
    return [mn for mn in mns if len(hist[mn['collateralHash']]) == 1]

def filtermulticollateraladdress(mns):
    '''Filter out MNs sharing the same collateral address'''
    hist = collections.defaultdict(list)
    for mn in mns:
        hist[mn['collateralAddress']].append(mn)
    return [mn for mn in mns if len(hist[mn['collateralAddress']]) == 1]

def filtermultipayoutaddress(mns):
    '''Filter out MNs sharing the same payout address'''
    hist = collections.defaultdict(list)
    for mn in mns:
        hist[mn['state']['payoutAddress']].append(mn)
    return [mn for mn in mns if len(hist[mn['state']['payoutAddress']]) == 1]

def resolveasn(resolver, ip):
    asn = int([x.to_text() for x in resolver.resolve('.'.join(reversed(ip.split('.'))) + '.origin.asn.cymru.com', 'TXT').response.answer][0].split('\"')[1].split(' ')[0])
    return asn

# Based on Greg Maxwell's seed_filter.py
def filterbyasn(ips, max_per_asn, max_total):
    # Sift out ips by type
    ips_ipv4 = [ip for ip in ips if ip['net'] == 'ipv4']
    ips_ipv6 = [ip for ip in ips if ip['net'] == 'ipv6']
    ips_onion = [ip for ip in ips if ip['net'] == 'onion']

    my_resolver = dns.resolver.Resolver()

    pool = multiprocessing.Pool(processes=16)

    # OpenDNS servers
    my_resolver.nameservers = ['208.67.222.222', '208.67.220.220']

    # Resolve ASNs in parallel
    asns = [pool.apply_async(resolveasn, args=(my_resolver, ip['ip'])) for ip in ips_ipv4]

    # Filter IPv4 by ASN
    result = []
    asn_count = {}
    for i in range(len(ips_ipv4)):
        ip = ips_ipv4[i]
        if len(result) == max_total:
            break
        try:
            asn = asns[i].get()
            if asn not in asn_count:
                asn_count[asn] = 0
            if asn_count[asn] == max_per_asn:
                continue
            asn_count[asn] += 1
            result.append(ip)
        except:
            sys.stderr.write('ERR: Could not resolve ASN for "' + ip['ip'] + '"\n')

    # TODO: filter IPv6 by ASN

    # Add back non-IPv4
    result.extend(ips_ipv6)
    result.extend(ips_onion)
    return result

def main():
    # This expects a json as outputted by "protx list valid 1"
    if len(sys.argv) > 1:
        with open(sys.argv[1], 'r', encoding="utf8") as f:
            mns = json.load(f)
    else:
        mns = json.load(sys.stdin)

    # Skip PoSe banned MNs
    mns = [mn for mn in mns if mn['state']['PoSeBanHeight'] == -1]
    # Skip MNs with < 10000 confirmations
    mns = [mn for mn in mns if mn['confirmations'] >= 10000]
    # Filter out MNs which are definitely from the same person/operator
    mns = filtermulticollateralhash(mns)
    mns = filtermulticollateraladdress(mns)
    mns = filtermultipayoutaddress(mns)
    # Extract IPs
    ips = [parseip(mn['state']['service']) for mn in mns]
    # Look up ASNs and limit results, both per ASN and globally.
    ips = filterbyasn(ips, MAX_SEEDS_PER_ASN, NSEEDS)
    # Sort the results by IP address (for deterministic output).
    ips.sort(key=lambda x: (x['net'], x['sortkey']))

    for ip in ips:
        if ip['net'] == 'ipv6':
            print('[%s]:%i' % (ip['ip'], ip['port']))
        else:
            print('%s:%i' % (ip['ip'], ip['port']))

if __name__ == '__main__':
    main()
