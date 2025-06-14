#!/usr/bin/env python3
# Copyright 2014 BitPay Inc.
# Copyright 2016-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
"""Exercise the utils via json-defined tests."""

from test_framework.test_framework import BitcoinTestFramework

import difflib
import json
import logging
import os
import subprocess
from pathlib import Path


class ToolUtils(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0  # No node/datadir needed

    def setup_network(self):
        pass

    def skip_test_if_missing_module(self):
        self.skip_if_no_bitcoin_tx()
        self.skip_if_no_bitcoin_util()

    def run_test(self):
        self.testcase_dir = Path(self.config["environment"]["SRCDIR"]) / "test" / "functional" / "data" / "util"
        self.bins = self.get_binaries()
        with open(self.testcase_dir / "bitcoin-util-test.json", encoding="utf8") as f:
            input_data = json.loads(f.read())

        for i, test_obj in enumerate(input_data):
            self.log.debug(f"Running [{i}]: " + test_obj["description"])
            self.test_one(test_obj)

    def test_one(self, testObj):
        """Runs a single test, comparing output and RC to expected output and RC.

        Raises an error if input can't be read, executable fails, or output/RC
        are not as expected. Error is caught by bctester() and reported.
        """
        # Get the exec names and arguments
        if testObj["exec"] == "./bitcoin-util":
            execrun = self.bins.util_argv() + testObj["args"]
        elif testObj["exec"] == "./bitcoin-tx":
            execrun = self.bins.tx_argv() + testObj["args"]

        # Read the input data (if there is any)
        inputData = None
        if "input" in testObj:
            with open(self.testcase_dir / testObj["input"], encoding="utf8") as f:
                inputData = f.read()

        # Read the expected output data (if there is any)
        outputFn = None
        outputData = None
        outputType = None
        if "output_cmp" in testObj:
            outputFn = testObj['output_cmp']
            outputType = os.path.splitext(outputFn)[1][1:]  # output type from file extension (determines how to compare)
            try:
                with open(self.testcase_dir / outputFn, encoding="utf8") as f:
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
        try:
            res = subprocess.run(execrun, capture_output=True, text=True, input=inputData)
        except OSError:
            logging.error("OSError, Failed to execute " + str(execrun))
            raise

        if outputData:
            data_mismatch, formatting_mismatch = False, False
            # Parse command output and expected output
            try:
                a_parsed = parse_output(res.stdout, outputType)
            except Exception as e:
                logging.error(f"Error parsing command output as {outputType}: '{str(e)}'; res: {str(res)}")
                raise
            try:
                b_parsed = parse_output(outputData, outputType)
            except Exception as e:
                logging.error('Error parsing expected output %s as %s: %s' % (outputFn, outputType, e))
                raise
            # Compare data
            if a_parsed != b_parsed:
                logging.error(f"Output data mismatch for {outputFn} (format {outputType}); res: {str(res)}")
                data_mismatch = True
            # Compare formatting
            if res.stdout != outputData:
                error_message = f"Output formatting mismatch for {outputFn}:\nres: {str(res)}\n"
                error_message += "".join(difflib.context_diff(outputData.splitlines(True),
                                                              res.stdout.splitlines(True),
                                                              fromfile=outputFn,
                                                              tofile="returned"))
                logging.error(error_message)
                formatting_mismatch = True

            assert not data_mismatch and not formatting_mismatch

        # Compare the return code to the expected return code
        wantRC = 0
        if "return_code" in testObj:
            wantRC = testObj['return_code']
        if res.returncode != wantRC:
            logging.error(f"Return code mismatch for {outputFn}; res: {str(res)}")
            raise Exception

        if "error_txt" in testObj:
            want_error = testObj["error_txt"]
            # A partial match instead of an exact match makes writing tests easier
            # and should be sufficient.
            if want_error not in res.stderr:
                logging.error(f"Error mismatch:\nExpected: {want_error}\nReceived: {res.stderr.rstrip()}\nres: {str(res)}")
                raise Exception
        else:
            if res.stderr:
                logging.error(f"Unexpected error received: {res.stderr.rstrip()}\nres: {str(res)}")
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


if __name__ == "__main__":
    ToolUtils(__file__).main()
