# Copyright 2014 BitPay, Inc.
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import subprocess
import os
import json
import sys

def bctest(testDir, testObj):
	execargs = testObj['exec']

	stdinCfg = None
	inputData = None
	if "input" in testObj:
		filename = testDir + "/" + testObj['input']
		inputData = open(filename).read()
		stdinCfg = subprocess.PIPE

	outputFn = testObj['output_cmp']
	outputData = open(testDir + "/" + outputFn).read()

	proc = subprocess.Popen(execargs, stdin=stdinCfg, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	try:
		outs = proc.communicate(input=inputData)
	except OSError:
		print("OSError, Failed to execute " + execargs[0])
		sys.exit(1)

	if outs[0] != outputData:
		print("Output data mismatch for " + outputFn)
		sys.exit(1)

def bctester(testDir, input_basename):
	input_filename = testDir + "/" + input_basename
	raw_data = open(input_filename).read()
	input_data = json.loads(raw_data)

	for testObj in input_data:
		bctest(testDir, testObj)

	sys.exit(0)

