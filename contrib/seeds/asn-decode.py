#!/usr/bin/env python3
# Copyright (c) 2013-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Class object for loading and decoding ASN parameters
#

import os
import re

class ASNParser:
    def __init__(self, asnTSVf = 'ip2asn.tsv', debug = True):
        self.asnMemDB = None
        self.asnFile = None
        self.debug = debug
        self.errors = []
        if(os.path.exists(f'{os.path.dirname(__file__)}/{asnTSVf}')):
            self.asnFile = f'{os.path.dirname(__file__)}/{asnTSVf}'
        else:
            raise ValueError(f'Input ip46->ASN tsv does not exist at location: {os.path.dirname(__file__)}/{asnTSVf}')
        self.buildIpAsnMemoryDB()

    def buildIpAsnMemoryDB(self):
        """Converts a TSV IP Range to ASN file to an in memory database.
        Args:
            self
        Returns:
            self
        Raises:
            ValueError: If the input file is corrupt or invalid.
        """
        with open(self.asnFile) as asnList:
            for idx, row in enumerate(asnList.readlines()):
                rowData = re.split(r'\t+',row.rstrip())
                if(len(rowData) != 5):
                    raise ValueError(f'Input file contains an error on line {idx}')
                self.ipV46Validator(rowData[0])
    def ipV46Validator(self, ipaddressString):
        """Calculate the square root of a number.
        Args:
            paddressString: A string representation of the ip address to
        Returns:
            integer: An integer representing the ip address version that was parsed.
        Raises:
            ValueError: ip address string is not a valid string
        """
        version = None
        ipAddressArr = None
        ipAddressInt = None
        if(len(ipaddressString.split(':'))>1):
            version = 6
            ipAddress128bit = 0
            intIpAddress = []
            if '::' in ipaddressString:
                pass
            else:
                fullIpV6Arr = ipaddressString.split(':')
                if(len(fullIpV6Arr) == 8):
                    for i in range(0,8):
                        intVal = int(fullIpV6Arr[i],16)
                        intIpAddress.append(intVal)
                        ipAddress128bit += intVal << 16*(7-i)
                else:
                    raise ValueError(f'IPV6 Address contains an incorrect number of segments: {ipaddressString}')
                print(intIpAddress)
                print(ipAddress128bit)
                print(ipaddressString)
        else:
            version = 4
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
                        if i == 0:
                            ipAddress32bit += intIpAddress[i] << 24
                        elif i == 1:
                            ipAddress32bit += intIpAddress[i] << 16
                        elif i == 2:
                            ipAddress32bit += intIpAddress[i] << 8
                        elif i == 3:
                            ipAddress32bit += intIpAddress[i]
                        else:
                            raise ValueError(f'Parsed ip address segement index out of range')
                    ipAddressArr = intIpAddress
                    ipAddressInt = ipAddress32bit
                else:
                    raise ValueError(f'Number of IPV4 segements does not equal 4:{ipaddressString}')
        return(version,ipAddressArr,ipAddressInt)

    def __del__(self):
        if(self.debug):
            print(f'Finished with {len(self.errors)} errors: {self.errors}')

if __name__ == '__main__':
    ip2asn = ASNParser()
    print(f"Welcome to the Python IP-ASN Converter")
