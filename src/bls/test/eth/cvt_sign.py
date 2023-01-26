import re
import os
import glob
import sys

RE_SEC = re.compile(r"privkey: '0x([a-f0-9]*)'")
RE_MSG = re.compile(r"message: '0x([a-f0-9]*)'")
RE_OUT = re.compile(r"output: '0x([a-f0-9]*)'")

def cvt_sign(fileName):
	with open(fileName) as f:
		s = f.read()
		sec = RE_SEC.search(s)
		msg = RE_MSG.search(s)
		out = RE_OUT.search(s)
		if not (sec and msg and out):
			raise Exception("not found")
		print("sec", sec.group(1))
		print("msg", msg.group(1))
		print("out", out.group(1))


if len(sys.argv) == 1:
	print('specify dir')
	sys.exit(1)
dir = sys.argv[1]

fs = glob.glob(dir + '/*/*.yaml')
for f in fs:
	cvt_sign(f)

