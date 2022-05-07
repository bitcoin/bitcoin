"""
POC of a function to load "contrib/signet/miner" as a module without having to
rename it to miner.py.

In the long run, this will ease to upgrade to newer bitcoin-core versions,
because we can directly modify the miner in-place.

ITCOIN_SPECIFIC
"""

import importlib.machinery
import importlib.util
from pathlib import Path
import sys
import types

def import_module_by_abspath(path_to_source) -> types.ModuleType:
    """Dynamically imports a python source file and returns it as a module.

    path_to_source must be an absolute path to an existing python file. It is
    not necessary that its name ends with ".py". The path is canonicalized,
    resolving any ".." and symlinks. It is then loaded as a module, added to
    sys.modules[DYNLOADED_FROM_<canonicalized_path_to_source>], and returned by
    the function.

    Inspired by:
    - https://csatlas.com/python-import-file-module/
    - https://docs.python.org/3/library/importlib.html#importing-a-source-file-directly

    EXAMPLE:
        # Fancy version of "import my_module", where my_module is taken from
        # "/etc/some_python_file":

        my_module = import_module_by_abspath('/etc/some_python_file')
        my_module.some_function()
    """
    p = Path(path_to_source)

    if p.is_absolute() == False:
        raise ValueError(f'path_to_source must be an absolute path. "{path_to_source} is not"')

    canonical_path = p.resolve(strict=True)
    MODULE_NAME = f'DYNLOADED_FROM_{canonical_path}'

    if MODULE_NAME in sys.modules:
        # do not load the same file again
        #
        # We cannot print anything on stderr, otherwise the bitcoin test
        # framework thinks tests have failed.
        #print(f'Module "{MODULE_NAME}" is already loaded. Not reloading.', file=sys.stderr)
        return sys.modules[MODULE_NAME]

    spec = importlib.util.spec_from_loader(
        MODULE_NAME,
        importlib.machinery.SourceFileLoader(
            MODULE_NAME,
            str(canonical_path),
        ),
    )
    module = importlib.util.module_from_spec(spec)
    sys.modules[MODULE_NAME] = module
    spec.loader.exec_module(module)
    return module

def import_miner() ->  types.ModuleType:
    """Looks for the "miner" file and imports it as a module.

    The lookup is performed on two possible hardcoded paths, accounting for
    whether the code is running:
    - in dev mode from a code checkout: the miner will be in
      <base>/contrib/signet
    - in production mode: the miner will be together with the other bitcoin
      binaries (like bitcoind, bitcoin-cli), and test_framework will be a
      subdirectory there
    """
    search_dirs = [
        "../../../contrib/signet/miner",
        "../../miner",
    ]
    path_to_this_file = Path(__file__).parent.resolve()
    possible_abs_paths = [
        (path_to_this_file / search_dir).resolve()
        for search_dir in search_dirs
    ]
    for path_to_miner in possible_abs_paths:
        if path_to_miner.is_file() == False:
            continue
        return import_module_by_abspath(path_to_miner)

    raise FileNotFoundError(f'Cannot find miner source code. Looked in: {possible_abs_paths}')
