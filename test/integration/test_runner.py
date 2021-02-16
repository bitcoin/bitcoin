import pathlib
import sys

from pypoptools.pypoptesting.framework.node import Node
from pypoptools.pypoptesting.framework.test_util import run_tests
from pypoptools.pypoptesting.tests import all_tests
from vbitcoind_node import VBitcoindNode


def create_node(number: int, path: pathlib.Path) -> Node:
    return VBitcoindNode(number=number, datadir=path)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        all_tests_by_name = dict([(test.name(), test) for test in all_tests])
        tests = [all_tests_by_name[name] for name in sys.argv[1:]]
    else:
        tests = all_tests
    run_tests(tests, create_node)
