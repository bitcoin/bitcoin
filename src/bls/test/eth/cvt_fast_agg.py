import re
import os
import glob
import sys

RE_HEX = re.compile(r"'0x([a-f0-9]*)'")
RE_PUB = re.compile(r"pubkeys: '0x([a-f0-9]*)'")
RE_MSG = re.compile(r"message: '0x([a-f0-9]*)'")
RE_SIG = re.compile(r"signature: '0x([a-f0-9]*)'")
RE_OUT = re.compile(r"output: ([a-z0-9]*)")

def cvt_sign(fileName):
	with open(fileName) as f:
		status = "in"
		for line in f:
			if status == "in":
				if line.find("message") > 0:
					print("msg", RE_HEX.search(line).group(1))
					status = "sig"
				else:
					r = RE_HEX.search(line)
					if r:
						print("pub", r.group(1))
			elif status == "sig":
				r = RE_HEX.search(line)
				print("sig", RE_HEX.search(line).group(1))
				status = "out"
			else:
				print("out", RE_OUT.search(line).group(1))


if len(sys.argv) == 1:
	print('specify dir')
	sys.exit(1)
dir = sys.argv[1]

fs = glob.glob(dir + '*/*.yaml')
for f in fs:
	cvt_sign(f)

