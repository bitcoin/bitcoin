#!/usr/bin/env python3
# Copyright 2014 BitPay Inc.
# Copyright 2016-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test framework for bitcoin utils.

Runs automatically during `make check`.

Can also be run manually."""

import argparse
import configparser
import difflib
import json
import logging
import os
import pprint
import re
import subprocess
import sys

def main():
    config = configparser.ConfigParser()
    config.optionxform = str
    with open(os.path.join(os.path.dirname(__file__), "../config.ini"), encoding="utf8") as f:
        config.read_file(f)
    env_conf = dict(config.items('environment'))

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-v', '--verbose', action='store_true')
    args = parser.parse_args()
    verbose = args.verbose

    if verbose:
        level = logging.DEBUG
    else:
        level = logging.WARNING
    formatter = '%(asctime)s - %(levelname)s - %(message)s'
    # Add the format/level to the logger
    logging.basicConfig(format=formatter, level=level)

    bctester(os.path.join(env_conf["SRCDIR"], "test", "util", "data"), "bitcoin-util-test.json", env_conf, config['components'])

def bctester(testDir, input_basename, buildenv, component_conf):
    """ Loads and parses the input file, runs all tests and reports results"""
    input_filename = os.path.join(testDir, input_basename)
    with open(input_filename, encoding="utf8") as f:
        raw_data = f.read()
    input_data = json.loads(raw_data)

    failed_testcases = []
    skipped_testcases = []
    skipped_testcase_deps = set()

    for testObj in input_data:
        m = re.match(r'^\.\/bitcoin-(\w+)$', testObj['exec'])
        if not component_conf.getboolean(f'ENABLE_UTIL_{m.group(1).upper()}'):
            logging.info("SKIPPED: " + testObj["description"])
            skipped_testcases.append(testObj['description'])
            skipped_testcase_deps.add(testObj['exec'])
            continue

        try:
            bctest(testDir, testObj, buildenv)
            logging.info("PASSED: " + testObj["description"])
        except Exception:
            logging.info("FAILED: " + testObj["description"])
            failed_testcases.append(testObj["description"])

    if failed_testcases:
        error_message = "FAILED_TESTCASES:\n"
        error_message += pprint.pformat(failed_testcases, width=400)
        logging.error(error_message)
        sys.exit(1)
    else:
        if skipped_testcases:
            logging.warning(f'{len(skipped_testcases)} tests skipped because {skipped_testcase_deps} is not built')
        sys.exit(0)

def bctest(testDir, testObj, buildenv):
    """Runs a single test, comparing output and RC to expected output and RC.

    Raises an error if input can't be read, executable fails, or output/RC
    are not as expected. Error is caught by bctester() and reported.
    """
    # Get the exec names and arguments
    execprog = os.path.join(buildenv["BUILDDIR"], "src", testObj["exec"] + buildenv["EXEEXT"])
    if testObj["exec"] == "./bitcoin-util":
        execprog = os.getenv("BITCOINUTIL", default=execprog)
    elif testObj["exec"] == "./bitcoin-tx":
        execprog = os.getenv("BITCOINTX", default=execprog)

    execargs = testObj['args']
    execrun = [execprog] + execargs

    # Read the input data (if there is any)
    stdinCfg = None
    inputData = None
    if "input" in testObj:
        filename = os.path.join(testDir, testObj["input"])
        with open(filename, encoding="utf8") as f:
            inputData = f.read()
        stdinCfg = subprocess.PIPE

    # Read the expected output data (if there is any)
    outputFn = None
    outputData = None
    outputType = None
    if "output_cmp" in testObj:
        outputFn = testObj['output_cmp']
        outputType = os.path.splitext(outputFn)[1][1:]  # output type from file extension (determines how to compare)
        try:
            with open(os.path.join(testDir, outputFn), encoding="utf8") as f:
                outputData = f.read()
        except Exception:
            logging.error("Output file " + outputFn + " cannot be opened")
            raise
        if not outputData:
            logging.error("Output data missing for " + outputFn)
            raise Exception
        if not outputType:
            logging.error("Output file %s does not have a file extension" % outputFn)
            raise Exception

    # Run the test
    proc = subprocess.Popen(execrun, stdin=stdinCfg, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        outs = proc.communicate(input=inputData)
    except OSError:
        logging.error("OSError, Failed to execute " + execprog)
        raise

    if outputData:
        data_mismatch, formatting_mismatch = False, False
        # Parse command output and expected output
        try:
            a_parsed = parse_output(outs[0], outputType)
        except Exception as e:
            logging.error('Error parsing command output as %s: %s' % (outputType, e))
            raise
        try:
            b_parsed = parse_output(outputData, outputType)
        except Exception as e:
            logging.error('Error parsing expected output %s as %s: %s' % (outputFn, outputType, e))
            raise
        # Compare data
        if a_parsed != b_parsed:
            logging.error("Output data mismatch for " + outputFn + " (format " + outputType + ")")
            data_mismatch = True
        # Compare formatting
        if outs[0] != outputData:
            error_message = "Output formatting mismatch for " + outputFn + ":\n"
            error_message += "".join(difflib.context_diff(outputData.splitlines(True),
                                                          outs[0].splitlines(True),
                                                          fromfile=outputFn,
                                                          tofile="returned"))
            logging.error(error_message)
            formatting_mismatch = True

        assert not data_mismatch and not formatting_mismatch

    # Compare the return code to the expected return code
    wantRC = 0
    if "return_code" in testObj:
        wantRC = testObj['return_code']
    if proc.returncode != wantRC:
        logging.error("Return code mismatch for " + outputFn)
        raise Exception

    if "error_txt" in testObj:
        want_error = testObj["error_txt"]
        # Compare error text
        # TODO: ideally, we'd compare the strings exactly and also assert
        # That stderr is empty if no errors are expected. However, bitcoin-tx
        # emits DISPLAY errors when running as a windows application on
        # linux through wine. Just assert that the expected error text appears
        # somewhere in stderr.
        if want_error not in outs[1]:
            logging.error("Error mismatch:\n" + "Expected: " + want_error + "\nReceived: " + outs[1].rstrip())
            raise Exception

def parse_output(a, fmt):
    """Parse the output according to specified format.

    Raise an error if the output can't be parsed."""
    if fmt == 'json':  # json: compare parsed data
        return json.loads(a)
    elif fmt == 'hex':  # hex: parse and compare binary data
        return bytes.fromhex(a.strip())
    else:
        raise NotImplementedError("Don't know how to compare %s" % fmt)

if __name__ == '__main__':
    main()
