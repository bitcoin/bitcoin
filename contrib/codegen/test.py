#!/usr/bin/env python3

import os
import tempfile
import json
import unittest
from subprocess import run, PIPE
from generate_client import load_schema, generate_client, sanitize_argument_names

class TestGenerateClient(unittest.TestCase):

    def setUp(self):
        # Create a temporary directory for output files
        self.temp_dir = tempfile.TemporaryDirectory()
        # Create a sample schema dictionary
        self.sample_schema = {
            "rpcs": {
                "getblockchaininfo": {
                    "description": "Returns blockchain info.",
                    "argument_names": []
                },
                "addconnection": {
                    "description": "Opens an outbound connection.",
                    "argument_names": ["address", "connection_type", "v2transport"]
                }
            }
        }
        # Sanitize the sample schema if necessary
        self.sample_schema["rpcs"] = sanitize_argument_names(self.sample_schema["rpcs"])

    def tearDown(self):
        # Clean up temporary directory
        self.temp_dir.cleanup()

    def test_generate_cpp_client(self):
        # Test generating C++ code
        output_dir = self.temp_dir.name
        generate_client(self.sample_schema, "cpp", output_dir)
        generated_file = os.path.join(output_dir, "bitcoin_rpc_client.cpp")
        self.assertTrue(os.path.exists(generated_file))
        with open(generated_file, "r") as f:
            content = f.read()
        # Check that key parts appear in the generated file
        self.assertIn("class BitcoinRPCClient", content)
        self.assertIn("std::string getblockchaininfo()", content)
        self.assertIn("std::string addconnection(", content)

    def test_generate_python_client(self):
        # Test generating Python code
        output_dir = self.temp_dir.name
        generate_client(self.sample_schema, "python", output_dir)
        generated_file = os.path.join(output_dir, "bitcoin_rpc_client.py")
        self.assertTrue(os.path.exists(generated_file))
        with open(generated_file, "r") as f:
            content = f.read()
        # Check that key parts appear in the generated file
        self.assertIn("class BitcoinRPCClient", content)
        self.assertIn("def getblockchaininfo(self):", content)
        self.assertIn("def addconnection(self, address, connection_type, v2transport):", content)

if __name__ == '__main__':
    unittest.main()
