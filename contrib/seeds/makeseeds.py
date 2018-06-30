#!/usr/bin/env python3
# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Generate seeds.txt from masternode list
#

NSEEDS=512

MAX_SEEDS_PER_ASN=4

MIN_PROTOCOL_VERSION = 70208
MAX_LAST_SEEN_DIFF = 60 * 60 * 24 * 1 # 1 day
MAX_LAST_PAID_DIFF = 60 * 60 * 24 * 30 # 1 month

# These are hosts that have been observed to be behaving strangely (e.g.
# aggressively connecting to every node).
SUSPICIOUS_HOSTS = {
}

import re
import sys
import dns.resolver
import collections
import json
import time

PATTERN_IPV4 = re.compile(r"^((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})):(\d+)$")
PATTERN_IPV6 = re.compile(r"^\[([0-9a-z:]+)\]:(\d+)$")
PATTERN_ONION = re.compile(r"^([abcdefghijklmnopqrstuvwxyz234567]{16}\.onion):(\d+)$")

def parseline(line):
    # line format: status protocol payee lastseen activeseconds lastpaidtime lastpaidblock IP
    sline = line.split()

    m = PATTERN_IPV4.match(sline[7])
    sortkey = None
    ip = None
    if m is None:
        m = PATTERN_IPV6.match(sline[7])
        if m is None:
            m = PATTERN_ONION.match(sline[7])
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
        "status": sline[0],
        "protocol": int(sline[1]),
        "payee": sline[2],
        "lastseen": int(sline[3]),
        "activeseconds": int(sline[4]),
        "lastpaidtime": int(sline[5]),
        "lastpaidblock": int(sline[6]),
        "net": net,
        "ip": ipstr,
        "port": port,
        "ipnum": ip,
        "sortkey": sortkey
    }

def filtermultiport(ips):
    '''Filter out hosts with more nodes per IP'''
    hist = collections.defaultdict(list)
    for ip in ips:
        hist[ip['sortkey']].append(ip)
    return [value[0] for (key,value) in list(hist.items()) if len(value)==1]

# Based on Greg Maxwell's seed_filter.py
def filterbyasn(ips, max_per_asn, max_total):
    # Sift out ips by type
    ips_ipv4 = [ip for ip in ips if ip['net'] == 'ipv4']
    ips_ipv6 = [ip for ip in ips if ip['net'] == 'ipv6']
    ips_onion = [ip for ip in ips if ip['net'] == 'onion']

    my_resolver = dns.resolver.Resolver()

    # OpenDNS servers
    my_resolver.nameservers = ['208.67.222.222', '208.67.220.220']

    # Filter IPv4 by ASN
    result = []
    asn_count = {}
    for ip in ips_ipv4:
        if len(result) == max_total:
            break
        try:
            asn = int([x.to_text() for x in my_resolver.query('.'.join(reversed(ip['ip'].split('.'))) + '.origin.asn.cymru.com', 'TXT').response.answer][0].split('\"')[1].split(' ')[0])
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
    js = json.load(sys.stdin)
    ips = [parseline(line) for collateral, line in js.items()]

    cur_time = int(time.time())

    # Skip entries with valid address.
    ips = [ip for ip in ips if ip is not None]
    # Enforce ENABLED state
    ips = [ip for ip in ips if ip['status'] == "ENABLED"]
    # Enforce minimum protocol version
    ips = [ip for ip in ips if ip['protocol'] >= MIN_PROTOCOL_VERSION]
    # Require at least 2 week uptime
    ips = [ip for ip in ips if cur_time - ip['lastseen'] < MAX_LAST_SEEN_DIFF]
    # Require to be paid recently
    ips = [ip for ip in ips if cur_time - ip['lastpaidtime'] < MAX_LAST_PAID_DIFF]
    # Sort by availability (and use lastpaidtime as tie breaker)
    ips.sort(key=lambda x: (x['activeseconds'], x['lastpaidtime'], x['ip']), reverse=True)
    # Filter out hosts with multiple ports, these are likely abusive
    ips = filtermultiport(ips)
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
