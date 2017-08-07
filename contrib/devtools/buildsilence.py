#!/usr/bin/python
# output . rather than the build output, unless something interesting is printed
# this script cleans up the travis build output
import sys
count = 0

while True:
    line = sys.stdin.readline()
    if line == "": break
    txt = line.strip()
    if len(txt) >=2 and txt[0:2] == "AR":
        sys.stdout.write(".")
        sys.stdout.flush()
        count += 1
    elif len(txt) >= 3 and txt[0:3] == "CXX":
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

