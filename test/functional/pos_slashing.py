#!/usr/bin/env python3
"""Exercise the slashing tracker to detect double-signing."""

import ctypes
import subprocess
from pathlib import Path

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class PosSlashingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0

    def run_test(self):
        repo_root = Path(__file__).resolve().parents[2]
        workdir = Path(self.options.tmpdir)
        wrapper = workdir / "slashing_wrap.cpp"
        sofile = workdir / "libslashing.so"
        wrapper.write_text(
            '#include "pos/slashing.h"\n'
            "extern \"C\" {\n"
            "pos::SlashingTracker* slashing_create(){return new pos::SlashingTracker();}\n"
            "void slashing_destroy(pos::SlashingTracker* t){delete t;}\n"
            "bool slashing_detect(pos::SlashingTracker* t,const char* v){return t->DetectDoubleSign(std::string{v});}\n"
            "}\n"
        )
        subprocess.check_call(
            [
                "g++",
                "-std=c++17",
                "-shared",
                "-fPIC",
                str(repo_root / "src/pos/slashing.cpp"),
                str(wrapper),
                "-I" + str(repo_root / "src"),
                "-o",
                str(sofile),
            ]
        )
        lib = ctypes.CDLL(str(sofile))
        lib.slashing_create.restype = ctypes.c_void_p
        lib.slashing_detect.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.slashing_detect.restype = ctypes.c_bool
        lib.slashing_destroy.argtypes = [ctypes.c_void_p]
        tracker = lib.slashing_create()
        assert_equal(lib.slashing_detect(tracker, b"validator"), False)
        assert_equal(lib.slashing_detect(tracker, b"validator"), True)
        lib.slashing_destroy(tracker)


if __name__ == "__main__":
    PosSlashingTest(__file__).main()
