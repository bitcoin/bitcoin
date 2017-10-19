#!/usr/bin/python
# output . rather than the build output, unless something interesting is printed
# this script cleans up the travis build output
import sys
count = 0

dotMatches = [ "AR", "CXX", "make", "libtool"]

while True:
    line = sys.stdin.readline()
    if line == "": break
    txt = line.strip()
    matched = False
    for m in dotMatches:
        if txt.startswith(m):
            matched=True
            break
    if matched:
        sys.stdout.write(".")
        sys.stdout.flush()
        count += 1
    else:
        if count:
            sys.stdout.write("\n")
        sys.stdout.write(line)
        count = 0
    if count > 80:
        sys.stdout.write("\n")
        count = 0

