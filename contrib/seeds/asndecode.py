#!/usr/bin/env python3
# Copyright (c) 2013-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Class object for loading and decoding ASN parameters
#

import os
import re
import sys
import requests
import gzip

class ASNParser:
    def __init__(self, asnFn = 'ip2asn.tsv.gz', fetchDb = True, asnFailoverLookup = True):
        self.asnDbURL = 'https://iptoasn.com/data/ip2asn-combined.tsv.gz'
        self.asnMemDB = []
        self.asnFileName = asnFn
        self.asnFile = None
        self.failover = asnFailoverLookup
        if fetchDb:
            self.fetchIpAsnDB()
        if(os.path.exists(f'{os.path.dirname(__file__)}/{asnFn}')):
            self.asnFile = f'{os.path.dirname(__file__)}/{asnFn}'
        else:
            raise ValueError(f'Input ip46->ASN tsv does not exist at location: {os.path.dirname(__file__)}/{asnFn}')
        self.buildIpAsnMemoryDB()

    def fetchIpAsnDB(self):
        '''
        Fetch the current ASN directory from an external http(s) source
        '''
        r = requests.get(self.asnDbURL)
        if(r.status_code != 200):
            raise ValueError(f'Unable to retrieve ASN DB from {self.asnDbURL} with status code {r.status_code}')
        with open(f'{os.path.dirname(__file__)}/{self.asnFileName}', 'wb') as f:
            f.write(r.content)

    def buildIpAsnMemoryDB(self):
        '''
        Constructs an in memory database that contains the asn directory from a filename
        '''
        with gzip.open(self.asnFile, 'rt') as asnList:
            for idx, row in enumerate(asnList.readlines()):
                rowData = re.split(r'\t+',row.rstrip())
                if(len(rowData) != 5):
                    raise ValueError(f'Input file contains an error on line {idx}')
                self.asnMemDB.append({
                    'version' : self.ipV46Validator(rowData[0])['version'],
                    'rangeStart' : self.ipV46Validator(rowData[0])['ip_int'],
                    'rangeEnd' : self.ipV46Validator(rowData[1])['ip_int'],
                    'asn' : int(rowData[2])
                })

    def ipV46Validator(self, ipaddressString):
        '''
        Validates,parses, and converts an ip address string into the following data
            - Detects the ip version as an intrger (4/6)
            - Converts the ip address into an integer 128bit/32bit
        '''
        ret = {
            'version' : None,
            'ip_int' : None
        }
        if(len(ipaddressString.split(':'))>1):
            ret['version'] = 6
            ipAddress128bit = 0
            intIpAddress = []
            if '::' in ipaddressString:
                intIpAddress = [0]*8
                fullIpV6Arr = ipaddressString.split('::')
                if(len(fullIpV6Arr) <= 2):#::prefix
                    if not fullIpV6Arr[0] and not fullIpV6Arr[1]:#empty::empty defaults to 0000:0000:0000:0000:0000:0000:0000:0000
                        pass
                    elif not fullIpV6Arr[0] and fullIpV6Arr[1]: #empty::ffff:nnnn
                        segments = fullIpV6Arr[1].split(':')
                        if(len(segments) > 8):
                            raise ValueError(f'IPV6 Contains too many segments: {ipaddressString}')
                        for idx, segment in enumerate(segments):
                            intIpAddress[-idx] = int(segment,16)
                    elif fullIpV6Arr[0] and not fullIpV6Arr[1]: #::suffix - most ASN start ranges will be in this format
                        segments = fullIpV6Arr[0].split(':')
                        if(len(segments) > 8):
                            raise ValueError(f'IPV6 Contains too many segments: {ipaddressString}')
                        for idx, segment in enumerate(segments): #Push segments as into the the defaulted int ipv6 array
                            intIpAddress[idx] = int(segment,16)
                    elif fullIpV6Arr[0] and fullIpV6Arr:
                        prefixSegments = fullIpV6Arr[0].split(':')
                        appendixSegments = fullIpV6Arr[1].split(':')
                        if(len(prefixSegments) > 8 or len(appendixSegments) > 8):
                            raise ValueError(f'IPV6 Contains too many segments: {ipaddressString}')
                        for idx, segment in enumerate(prefixSegments): #Push segments as into the the defaulted int ipv6 array prefix
                            intIpAddress[idx] = int(segment,16)
                        for idx, segment in enumerate(appendixSegments):
                            intIpAddress[idx-len(appendixSegments)] = int(segment,16)
                        #print(f'Prefix Segments: {prefixSegments} - Appendix Segments {appendixSegments} - Result {intIpAddress}', file=sys.stderr)
                    #Now take the base10 ipv6 int array and convert it into an integer value
                    for idx, segment in enumerate(intIpAddress): #Sum ipaddress into the int ipAddress
                        ipAddress128bit += segment << 16*(7-idx)
                    ret['ip_int'] = ipAddress128bit
                else: #for ipv6 address with a split in the middle such as ffff:eeee:dddd::0:1, this format is not commonly used, raise a value error
                    raise ValueError(f'IPV6 Address format of xxxx::yyyy not supported: {ipaddressString}')
            else:
                fullIpV6Arr = ipaddressString.split(':')
                if(len(fullIpV6Arr) == 8):
                    for idx in range(0,8):
                        intVal = int(fullIpV6Arr[idx],16)
                        intIpAddress.append(intVal)
                        ipAddress128bit += intVal << 16*(7-idx)
                    ret['ip_int'] = ipAddress128bit
                else:
                    raise ValueError(f'IPV6 Address contains an incorrect number of segments: {ipaddressString}')
            #Place the final base10 ip address value into the return array
        else:
            ret['version'] = 4
            asciiIPAddress = ipaddressString.split('.')
            if(len(asciiIPAddress) > 4):
                raise ValueError(f'IPV4 Address contains an incorrect number of segments: {ipaddressString}')
            else:
                intIpAddress = []
                ipAddress32bit = 0
                for segment in asciiIPAddress:
                    intSegment = int(segment)
                    if(intSegment > 255 or intSegment < 0):
                        raise ValueError(f'IPV4 segment contains a value larger than 255 or less than 0:{ipaddressString}')
                    else:
                        intIpAddress.append(intSegment)
                if len(intIpAddress) == 4:
                    for i in range(0,4):
                        ipAddress32bit += intIpAddress[i] << 24-(8*i)
                    ret['ip_int'] = ipAddress32bit
                else:
                    raise ValueError(f'Number of IPV4 segments does not equal 4:{ipaddressString}')
        if((ret['version'] is None) or (ret['ip_int'] is None)):
            raise ValueError(f'IP address decoding failed: {ipaddressString}')
        return ret

    def lookup_asn(self, net, ip):
        '''
        Look up the asn for an IPv(4/6) address by querying the local asn database
        that is fetched at runtime or loaded directly from a file. Supports failover
        to the lecacy API based method.
        '''
        if(len(self.asnMemDB) > 0):
            ipValid = self.ipV46Validator(ip)
            ipVersion = None
            asn = None
            if net == 'ipv4':
                ipVersion = 4
            else:
                ipVersion = 6
            for entry in self.asnMemDB:
                if ipValid['ip_int'] >= entry['rangeStart'] and ipValid['ip_int'] <= entry['rangeEnd'] and entry['version'] == ipVersion:
                    asn = int(entry['asn'])
                    break
            if(asn is None and self.failover):
                self.lookup_asn_api(net, ip)
            elif(asn is None and not self.failover):
                sys.stderr.write(f'ERR: Could not resolve ASN for "{ip}"\n')
            return asn
        else:
            raise ValueError(f'The asn registry database is empty.')

    def lookup_asn_api(self, net, ip):
        '''
        Look up the asn for an IP (4 or 6) address by querying cymru.com, or None
        if it could not be found.
        '''
        try:
            if net == 'ipv4':
                ipaddr = ip
                prefix = '.origin'
            else:                                   # http://www.team-cymru.com/IP-ASN-mapping.html
                res = str()                         # 2001:4860:b002:23::68
                for nb in ip.split(':')[:4]:        # pick the first 4 nibbles
                    for c in nb.zfill(4):           # right padded with '0'
                        res += c + '.'              # 2001 4860 b002 0023
                ipaddr = res.rstrip('.')            # 2.0.0.1.4.8.6.0.b.0.0.2.0.0.2.3
                prefix = '.origin6'

            asn = int([x.to_text() for x in dns.resolver.resolve('.'.join(
                        reversed(ipaddr.split('.'))) + prefix + '.asn.cymru.com',
                        'TXT').response.answer][0].split('\"')[1].split(' ')[0])
            return asn
        except Exception as e:
            sys.stderr.write(f'ERR: Could not resolve ASN for "{ip}": {e}\n')
            return None
