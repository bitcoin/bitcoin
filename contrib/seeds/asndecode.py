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

class ASNParser:
    def __init__(self, asnTSVf = 'ip2asn.tsv'):
        self.asnMemDB = []
        self.asnFile = None
        self.errors = []
        if(os.path.exists(f'{os.path.dirname(__file__)}/{asnTSVf}')):
            self.asnFile = f'{os.path.dirname(__file__)}/{asnTSVf}'
        else:
            raise ValueError(f'Input ip46->ASN tsv does not exist at location: {os.path.dirname(__file__)}/{asnTSVf}')
        self.buildIpAsnMemoryDB()

    def buildIpAsnMemoryDB(self):
        with open(self.asnFile) as asnList:
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
                        for idx, segment in enumerate(segments): #Push segements as into the the defauled int ipv6 array
                            intIpAddress[idx] = int(segment,16)
                    elif fullIpV6Arr[0] and fullIpV6Arr:
                        prefixSegments = fullIpV6Arr[0].split(':')
                        appendixSegments = fullIpV6Arr[1].split(':')
                        if(len(prefixSegments) > 8 or len(appendixSegments) > 8):
                            raise ValueError(f'IPV6 Contains too many segments: {ipaddressString}')
                        for idx, segment in enumerate(prefixSegments): #Push segements as into the the defauled int ipv6 array prefix
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
                        raise ValueError(f'IPV4 segement contains a value larger than 255 or less than 0:{ipaddressString}')
                    else:
                        intIpAddress.append(intSegment)
                if len(intIpAddress) == 4:
                    for i in range(0,4):
                        ipAddress32bit += intIpAddress[i] << 24-(8*i)
                    ret['ip_int'] = ipAddress32bit
                else:
                    raise ValueError(f'Number of IPV4 segements does not equal 4:{ipaddressString}')
        if((ret['version'] == None) or (ret['ip_int'] == None)):
            raise ValueError(f'IP address decoding failed: {ipaddressString}')
        return ret

    def lookup_asn(self, net, ip):
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
            if(asn == None):
                sys.stderr.write(f'ERR: Could not resolve ASN for "{ip}"\n')
            else:
                sys.stderr.write(ip)
            return asn
        else:
            raise ValueError(f'The asn registry database is empty.')

if __name__ == '__main__':
    ip2asn = ASNParser()
