# Copyright 2014 BitPay, Inc.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from __future__ import division,print_function,unicode_literals
import subprocess
import os
import json
import sys

def bctest(testDir, testObj, exeext):

	execprog = testObj['exec'] + exeext
	execargs = testObj['args']
	execrun = [execprog] + execargs
	stdinCfg = None
	inputData = None
	if "input" in testObj:
		filename = testDir + "/" + testObj['input']
		inputData = open(filename).read()
		stdinCfg = subprocess.PIPE

	outputFn = None
	outputData = None
	if "output_cmp" in testObj:
		outputFn = testObj['output_cmp']
		outputData = open(testDir + "/" + outputFn).read()
		if not outputData:
			print("Output data missing for " + outputFn)
			sys.exit(1)
	proc = subprocess.Popen(execrun, stdin=stdinCfg, stdout=subprocess.PIPE, stderr=subprocess.PIPE,universal_newlines=True)
	try:
		outs = proc.communicate(input=inputData)
	except OSError:
		print("OSError, Failed to execute " + execprog)
		sys.exit(1)

	if outputData and (outs[0] != outputData):
		print("Output data mismatch for " + outputFn)
		sys.exit(1)

	wantRC = 0
	if "return_code" in testObj:
		wantRC = testObj['return_code']
	if proc.returncode != wantRC:
		print("Return code mismatch for " + outputFn)
		sys.exit(1)

def bctester(testDir, input_basename, buildenv):
	input_filename = testDir + "/" + input_basename
	raw_data = open(input_filename).read()
	input_data = json.loads(raw_data)

	for testObj in input_data:
		bctest(testDir, testObj, buildenv.exeext)

	sys.exit(0)

