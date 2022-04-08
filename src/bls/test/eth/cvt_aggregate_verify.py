import re
import os
import glob
import sys

RE_HEX = re.compile(r"'0x([a-f0-9]*)'")
RE_PUB = re.compile(r"pubkeys: '0x([a-f0-9]*)'")
RE_MSG = re.compile(r"messages: '0x([a-f0-9]*)'")
RE_SIG = re.compile(r"signature: '0x([a-f0-9]*)'")
RE_OUT = re.compile(r"output: (true|false)")

def cvt_sign(fileName):
	with open(fileName) as f:
		text = f.read().replace(',', '\n').split('\n')
		status = "pubkeys"
		for line in text:
			if status == "pubkeys":
				if line.find("messages") > 0:
					if not RE_HEX.search(line):
						return
					print("msg", RE_HEX.search(line).group(1))
					status = "messages"
				else:
					r = RE_HEX.search(line)
					if r:
						print("pub", r.group(1))
			elif status == "messages":
				if line.find("signature") > 0:
					print("sig", RE_HEX.search(line).group(1))
					status = "output"
				else:
					r = RE_HEX.search(line)
					if r:
						print("msg", r.group(1))
			elif status == "sig":
				r = RE_HEX.search(line)
				print("sig", RE_HEX.search(line).group(1))
				status = "output"
			else:
				print("out", RE_OUT.search(line).group(1))
				status = "pubkeys"


if len(sys.argv) == 1:
	print('specify dir')
	sys.exit(1)
dir = sys.argv[1]

fs = glob.glob(dir + '*/*.yaml')
for f in fs:
	cvt_sign(f)

