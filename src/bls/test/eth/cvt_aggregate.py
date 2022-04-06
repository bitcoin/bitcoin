import re
import os
import glob
import sys

RE_HEX = re.compile(r"'0x([a-f0-9]*)'")
RE_PUB = re.compile(r"input: '0x([a-f0-9]*)'")
RE_OUT = re.compile(r"output: '0x([a-f0-9]*)'")

def cvt_sign(fileName):
	with open(fileName) as f:
		for line in f:
			if line.find("output") >= 0:
				r = RE_HEX.search(line)
				if r:
					print("out", r.group(1))
				return
			else:
				r = RE_HEX.search(line)
				if r:
					print("sig", r.group(1))


if len(sys.argv) == 1:
	print('specify dir')
	sys.exit(1)
dir = sys.argv[1]

fs = glob.glob(dir + '*/*.yaml')
for f in fs:
	cvt_sign(f)

