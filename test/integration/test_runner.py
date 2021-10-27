import pathlib
import sys

from pypoptools.pypoptesting.framework.node import Node
from pypoptools.pypoptesting.test_runner import tests_running
from btcsqd_node import BTCSQdNode


def create_node(number: int, path: pathlib.Path) -> Node:
    return BTCSQdNode(number=number, datadir=path)


if __name__ == '__main__':
    all_passed = tests_running(test_names=sys.argv[1:], create_node=create_node)
    sys.exit(not all_passed)
