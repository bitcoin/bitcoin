#!/usr/bin/env python

NSEEDS=600

import re
import sys
from subprocess import check_output

def main():
    lines = sys.stdin.readlines()

    ips = []
    pattern = re.compile(r"^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}):7777")
    for line in lines:
        m = pattern.match(line)
        if m is None:
            continue
        ip = 0
        for i in range(0,4):
            ip = ip + (int(m.group(i+1)) << (8*(i)))
        if ip == 0:
            continue
        ips.append(ip)

    for row in range(0, min(NSEEDS,len(ips)), 8):
        print "    " + ", ".join([ "0x%08x"%i for i in ips[row:row+8] ]) + ","

if __name__ == '__main__':
    main()
