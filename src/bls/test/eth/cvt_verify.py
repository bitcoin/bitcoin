import re
import os
import glob
import sys

RE_PUB = re.compile(r"pubkey: '0x([a-f0-9]*)'")
RE_MSG = re.compile(r"message: '0x([a-f0-9]*)'")
RE_SIG = re.compile(r"signature: '0x([a-f0-9]*)'")
RE_OUT = re.compile(r"output: (true|false)")

def cvt(fileName):
	with open(fileName) as f:
		s = f.read()
		pub = RE_PUB.search(s)
		msg = RE_MSG.search(s)
		sig = RE_SIG.search(s)
		out = RE_OUT.search(s)
		if not (pub and msg and sig and out):
			raise Exception("not found")
		print("pub", pub.group(1))
		print("msg", msg.group(1))
		print("sig", sig.group(1))
		print("out", out.group(1))


if len(sys.argv) == 1:
	print('specify dir')
	sys.exit(1)
dir = sys.argv[1]

fs = glob.glob(dir + '/*/*.yaml')
for f in fs:
	cvt(f)

