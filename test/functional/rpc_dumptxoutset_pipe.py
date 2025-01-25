from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
import os
import subprocess
import tempfile

class DumptxoutsetPipeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("Generating blocks to create a UTXO set...")
        self.nodes[0].generate(101) 

        self.log.info("Creating a named pipe...")
        with tempfile.TemporaryDirectory() as tmpdir:
            pipe_path = os.path.join(tmpdir, "utxo.pipe")
            os.mkfifo(pipe_path)  

            self.log.info("Running dumptxoutset with the named pipe...")
            dump_proc = subprocess.Popen(
                [self.nodes[0].bitcoin_cli, "dumptxoutset", pipe_path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )

            with open(pipe_path, "rb") as pipe:
                utxo_data = pipe.read()

            stdout, stderr = dump_proc.communicate()
            self.log.debug(f"RPC stdout: {stdout.decode()}")
            self.log.debug(f"RPC stderr: {stderr.decode()}")

            self.log.info("Validating the output...")
            assert dump_proc.returncode == 0, "dumptxoutset RPC failed"
            assert utxo_data.startswith(b"utxo"), "UTXO data not as expected"  

            os.unlink(pipe_path)

        self.log.info("Test completed successfully!")

if __name__ == "__main__":
    DumptxoutsetPipeTest().main()
