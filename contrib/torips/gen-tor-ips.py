#!/usr/bin/env python

# Script to generate a C++ source file containing known Tor exits.
# This is used to help nodes treat Tor traffic homogenously.

import urllib2, time

data = urllib2.urlopen("https://check.torproject.org/exit-addresses").read()
exitlines = [line for line in data.split('\n') if line.startswith("ExitAddress")]
ipstrs = [line.split()[1] for line in exitlines]
ipstrs.sort()

contents = """// Generated at %s by gen-tor-ips.py: DO NOT EDIT

static const char *pszTorExits[] = {
    "0.1.2.3",   // For unit testing
%s
    NULL
};
""" % (time.asctime(), "\n".join(["    \"%s\"," % ip for ip in ipstrs]))

print contents
