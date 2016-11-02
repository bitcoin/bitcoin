# Copyright 2014 BitPay, Inc.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from __future__ import division,print_function,unicode_literals
import subprocess
import os
import json
import sys
import binascii

def parse_output(a, fmt):
	if fmt == 'json': # json: compare parsed data
		return json.loads(a)
	elif fmt == 'hex': # hex: parse and compare binary data
		return binascii.a2b_hex(a.strip())
	else:
		raise NotImplementedError("Don't know how to compare %s" % fmt)

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
		outputType = os.path.splitext(outputFn)[1][1:] # output type from file extension (determines how to compare)
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

	if outputData:
		try:
			a_parsed = parse_output(outs[0], outputType)
		except Exception as e:
			print('Error parsing command output as %s: %s' % (outputType,e))
			sys.exit(1)
		try:
			b_parsed = parse_output(outputData, outputType)
		except Exception as e:
			print('Error parsing expected output %s as %s: %s' % (outputFn,outputType,e))
			sys.exit(1)
		if a_parsed != b_parsed:
			print("Output data mismatch for " + outputFn + " (format " + outputType + ")")
			sys.exit(1)
		if outs[0] != outputData:
			print("Output formatting mismatch for " + outputFn + " (format " + outputType + ")")
			sys.exit(1)

	wantRC = 0
	if "return_code" in testObj:
		wantRC = testObj['return_code']
	if proc.returncode != wantRC:
		print("Return code mismatch for " + outputFn)
		sys.exit(1)

def bctester(testDir, input_basename, buildenv, verbose = False):
	input_filename = testDir + "/" + input_basename
	raw_data = open(input_filename).read()
	input_data = json.loads(raw_data)

	for testObj in input_data:
		if verbose and "description" in testObj:
			print ("Testing: " + testObj["description"])
		bctest(testDir, testObj, buildenv.exeext)
		if verbose and "description" in testObj:
			print ("PASS")

	sys.exit(0)

