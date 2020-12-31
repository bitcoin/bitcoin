#!/usr/bin/env python3
# Copyright (c) 2013-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Generate seeds.txt from Pieter's DNS seeder
#

import re
import sys
import dns.resolver
import collections

NSEEDS=512

MAX_SEEDS_PER_ASN=2

MIN_BLOCKS = 337600

# These are hosts that have been observed to be behaving strangely (e.g.
# aggressively connecting to every node).
with open("suspicious_hosts.txt", mode="r", encoding="utf-8") as f:
    SUSPICIOUS_HOSTS = {s.strip() for s in f if s.strip()}


PATTERN_IPV4 = re.compile(r"^((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})):(\d+)$")
PATTERN_IPV6 = re.compile(r"^\[([0-9a-z:]+)\]:(\d+)$")
PATTERN_ONION = re.compile(r"^([abcdefghijklmnopqrstuvwxyz234567]{16}\.onion):(\d+)$")
PATTERN_AGENT = re.compile(
    r"^/Satoshi:("
    r"0.14.(0|1|2|3|99)|"
    r"0.15.(0|1|2|99)|"
    r"0.16.(0|1|2|3|99)|"
    r"0.17.(0|0.1|1|2|99)|"
    r"0.18.(0|1|99)|"
    r"0.19.(0|1|99)|"
    r"0.20.(0|1|99)|"
    r"0.21.99"
    r")")

def parseline(line):
    sline = line.split()
    if len(sline) < 11:
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

def dedup(ips):
    '''deduplicate by address,port'''
    d = {}
    for ip in ips:
        d[ip['ip'],ip['port']] = ip
    return list(d.values())

def filtermultiport(ips):
    '''Filter out hosts with more nodes per IP'''
    hist = collections.defaultdict(list)
    for ip in ips:
        hist[ip['sortkey']].append(ip)
    return [value[0] for (key,value) in list(hist.items()) if len(value)==1]

def lookup_asn(net, ip):
    '''
    Look up the asn for an IP (4 or 6) address by querying cymru.com, or None
    if it could not be found.
    '''
    try:
        if net == 'ipv4':
            ipaddr = ip
            prefix = '.origin'
        else:                  # http://www.team-cymru.com/IP-ASN-mapping.html
            res = str()                         # 2001:4860:b002:23::68
            for nb in ip.split(':')[:4]:  # pick the first 4 nibbles
                for c in nb.zfill(4):           # right padded with '0'
                    res += c + '.'              # 2001 4860 b002 0023
            ipaddr = res.rstrip('.')            # 2.0.0.1.4.8.6.0.b.0.0.2.0.0.2.3
            prefix = '.origin6'

        asn = int([x.to_text() for x in dns.resolver.resolve('.'.join(
                   reversed(ipaddr.split('.'))) + prefix + '.asn.cymru.com',
                   'TXT').response.answer][0].split('\"')[1].split(' ')[0])
        return asn
    except Exception:
        sys.stderr.write('ERR: Could not resolve ASN for "' + ip + '"\n')
        return None

# Based on Greg Maxwell's seed_filter.py
def filterbyasn(ips, max_per_asn, max_per_net):
    # Sift out ips by type
    ips_ipv46 = [ip for ip in ips if ip['net'] in ['ipv4', 'ipv6']]
    ips_onion = [ip for ip in ips if ip['net'] == 'onion']

    # Filter IPv46 by ASN, and limit to max_per_net per network
    result = []
    net_count = collections.defaultdict(int)
    asn_count = collections.defaultdict(int)
    for ip in ips_ipv46:
        if net_count[ip['net']] == max_per_net:
            continue
        asn = lookup_asn(ip['net'], ip['ip'])
        if asn is None or asn_count[asn] == max_per_asn:
            continue
        asn_count[asn] += 1
        net_count[ip['net']] += 1
        result.append(ip)

    # Add back Onions (up to max_per_net)
    result.extend(ips_onion[0:max_per_net])
    return result

def ip_stats(ips):
    hist = collections.defaultdict(int)
    for ip in ips:
        if ip is not None:
            hist[ip['net']] += 1

    return '%6d %6d %6d' % (hist['ipv4'], hist['ipv6'], hist['onion'])

def main():
    lines = sys.stdin.readlines()
    ips = [parseline(line) for line in lines]

    print('\x1b[7m  IPv4   IPv6  Onion Pass                                               \x1b[0m', file=sys.stderr)
    print('%s Initial' % (ip_stats(ips)), file=sys.stderr)
    # Skip entries with invalid address.
    ips = [ip for ip in ips if ip is not None]
    print('%s Skip entries with invalid address' % (ip_stats(ips)), file=sys.stderr)
    # Skip duplicates (in case multiple seeds files were concatenated)
    ips = dedup(ips)
    print('%s After removing duplicates' % (ip_stats(ips)), file=sys.stderr)
    # Skip entries from suspicious hosts.
    ips = [ip for ip in ips if ip['ip'] not in SUSPICIOUS_HOSTS]
    print('%s Skip entries from suspicious hosts' % (ip_stats(ips)), file=sys.stderr)
    # Enforce minimal number of blocks.
    ips = [ip for ip in ips if ip['blocks'] >= MIN_BLOCKS]
    print('%s Enforce minimal number of blocks' % (ip_stats(ips)), file=sys.stderr)
    # Require service bit 1.
    ips = [ip for ip in ips if (ip['service'] & 1) == 1]
    print('%s Require service bit 1' % (ip_stats(ips)), file=sys.stderr)
    # Require at least 50% 30-day uptime for clearnet, 10% for onion.
    req_uptime = {
        'ipv4': 50,
        'ipv6': 50,
        'onion': 10,
    }
    ips = [ip for ip in ips if ip['uptime'] > req_uptime[ip['net']]]
    print('%s Require minimum uptime' % (ip_stats(ips)), file=sys.stderr)
    # Require a known and recent user agent.
    ips = [ip for ip in ips if PATTERN_AGENT.match(ip['agent'])]
    print('%s Require a known and recent user agent' % (ip_stats(ips)), file=sys.stderr)
    # Sort by availability (and use last success as tie breaker)
    ips.sort(key=lambda x: (x['uptime'], x['lastsuccess'], x['ip']), reverse=True)
    # Filter out hosts with multiple bitcoin ports, these are likely abusive
    ips = filtermultiport(ips)
    print('%s Filter out hosts with multiple bitcoin ports' % (ip_stats(ips)), file=sys.stderr)
    # Look up ASNs and limit results, both per ASN and globally.
    ips = filterbyasn(ips, MAX_SEEDS_PER_ASN, NSEEDS)
    print('%s Look up ASNs and limit results per ASN and per net' % (ip_stats(ips)), file=sys.stderr)
    # Sort the results by IP address (for deterministic output).
    ips.sort(key=lambda x: (x['net'], x['sortkey']))
    for ip in ips:
        if ip['net'] == 'ipv6':
            print('[%s]:%i' % (ip['ip'], ip['port']))
        else:
            print('%s:%i' % (ip['ip'], ip['port']))

if __name__ == '__main__':
    main()
