#!/usr/bin/env python3

"""
# Automatic application of the "TS_ITCOIN_" prefix for bitcoin code base.

This tool takes:
1. a vanilla bitcoin source code
2. a `configure-itcoin-core-xxx.sh` script
3. a modified version of `src/threadsafety.h` where symbols have been prefixed
   with ""

And rewrites the code base adding the "TS_ITCOIN_" prefix where needed.

The tool can be used as a preparatory pass for modifying a new bitcoin version
before merging it back to itcoin.
For ease of reference this tool is committed in the "itcoin" branch, however it
is meant to be used from a vanilla bitcoin checkout.

# USAGE WORKFLOW

## Preparation

1. check out a "plain" bitcoin version (for example hg up --rev v23.0).
   This will be a standard, non-modified bitcoin-core source code. No itcoin
   specific modifications or scripts will be present.

2. create an empty "infra" directory at the repository root. This will be used
   only temporarily: its content will be deleted later on.
   ```mkdir <reporoot>/infra; cd infra```

3. from the "itcoin" branch or the repository, take the latest versions of
   "rename_threadsafety.py" and "configure-itcoin-core-dev.sh", and put them in
   the "infra" directory just created.
   A mercurial command for doing so would be:
   ```
   hg cat --rev itcoin infra/rename_threadsafety.py       > infra/rename_threadsafety.py
   hg cat --rev itcoin infra/configure-itcoin-core-dev.sh > infra/configure-itcoin-core-dev.sh
   ```

4. modify the <reporoot>/src/threadsafety.h adding the "TS_ITCOIN_" prefix where
   needed. You can start from the latest itcoin version
   (`hg cat --rev itcoin src/threadsafety.h`), but please be aware that you will
   need to manually forward port any new changes that may come from bitcoin.
   This is the only manual code modification you will be required to do.

## Running

1. Reconfigure the project:
   ```
   cd <reporoot>/infra
   rename_threadsafety.py reconfigure
   ```
   This will call "configure-itcoin-core.dev" with an additional configuration
   option that will instruct the compiler to generate diagnostic messages in
   json.

2. Run the tool:
   ```
   cd <reporoot>/infra
   rename_threadsafety.py run
   ```
   This will detect the modifications done in src/threadsafety.h, and perform as
   many compiler cycles as needed, changing the symbols as it goes. If the last
   cycle succeeds, the working copy will be modified adding the "TS_ITCOIN_"
   prefix where needed.

## Committing the work. Preparing to integrate a new bitcoin version

1. Remove the "infra" subdirectory (`rm -rf <reporoot>/infra`)

2. commit the changes. This will be a preparatory commit that can be merged back
   into the "itcoin" branch.

# Optional: reverting the changes.

If the modifications done by this tool become unnecessary, you may want to
revert them. Reverting the changes is easier than applying them, since you can
simply remove the "TS_ITCOIN_" prefix:

From a "itcoin" branch:
```
$ cd src
$ find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -print0  | xargs -I {} -0 sed --in-place 's/TS_ITCOIN_//g' {}
```

# Requirements
- python>=3.8
- gcc/g++ 11
- clang NOT supported

# Tests

## Doctests

The project has a minimal testing coverage via doctest:
```
python -m doctest rename_threadsafety.py
```

Python 3.10 improved pprint() support for dataclasses, but this broke doctests
on that python version. Please laungh doctests with python 3.8 or 3.9.

## MyPy

The type annotations are almost sound.
```
mypy rename_threadsafety.py
```
There is a minor problem in ts_error_from_gcc_error_raw_line_and_message() and
another one in ts_error_from_gcc_error_not_a_type_with_fixit().
"""

from dataclasses import dataclass
import dataclasses
from pathlib import Path
from typing import Any, Dict, List, Optional, Set, Tuple, Union
from pprint import pprint
import argparse
import copy
import itertools
import logging
import linecache
import os
import subprocess
import textwrap
import re
import json
from timeit import default_timer as timer

class PathEncoder(json.JSONEncoder):
     """JSON-encodes pathlib.Path objects with their string representation.

     USAGE:
         json.dumps(object, cls=PathEncoder)

     >>> from dataclasses import dataclass
     >>> from pathlib import Path
     >>> @dataclass
     ... class PathDC:
     ...     p: Path
     ...
     >>> dc = PathDC(Path('/'))

     >>> print(dc)
     PathDC(p=PosixPath('/'))

     >>> print(dataclasses.asdict(dc))
     {'p': PosixPath('/')}

     Without this class, trying to serialize to JSON a dataclass containing a
     pathlib.Path object causes an error:
     >>> print(json.dumps(dataclasses.asdict(dc)))
     Traceback (most recent call last):
     ...
     TypeError: Object of type PosixPath is not JSON serializable

     Using this class as encoder allows us to print the Path contents as string
     (useful for debugging purposes):
     >>> print(json.dumps(dataclasses.asdict(dc), cls=PathEncoder))
     {"p": "/"}
     """
     def default(self, obj):
          if isinstance(obj, Path):
               return str(obj)
          # Let the base class default method raise the TypeError
          return json.JSONEncoder.default(self, obj)

@dataclass
class RawGccError:
    kind: str
    caret_column: int
    file: Path
    line: int
    message: str
    raw_line: str
    other: Dict[str, Any]

    def __post_init__(self) -> None:
        if self.caret_column > len(self.raw_line):
            raise ValueError(f"Cannot initialize {self}. caret_column ({self.caret_column}) cannot be greater than raw_line's length ({len(self.raw_line)})")
        # we are now going to mutate self.other. Let's do a deep copy, otherwise
        # we would change the original dict
        self.other = copy.deepcopy(self.other)
        if 'kind' in self.other:
            del self.other['kind']
        if 'message' in self.other:
            del self.other['message']
        try:
            del self.other['locations'][0]['caret']['byte-column']
            del self.other['locations'][0]['caret']['column']
            del self.other['locations'][0]['caret']['display-column']
            del self.other['locations'][0]['caret']['file']
            del self.other['locations'][0]['caret']['line']
            if self.other['locations'][0]['caret'] == {}:
                del self.other['locations'][0]['caret']
            self._remove_locations_if_empty()
        except (IndexError, KeyError):
            pass

    def _remove_locations_if_empty(self) -> None:
        try:
            if self.other['locations'][0] == {}:
                del self.other['locations'][0]
            if self.other['locations'] == []:
                del self.other['locations']
        except (IndexError, KeyError):
            pass


@dataclass
class RawGccErrorRanged(RawGccError):
    finish_column: int

    def highlighted_string(self) -> str:
        """
        >>> e = RawGccErrorRanged('error', 16, Path('util/epochguard.h'), 33, "variable ‘LOCKABLE Epoch’ has initializer but incomplete type", "class LOCKABLE Epoch", {}, 20)
        >>> e.highlighted_string()
        'Epoch'
        """
        return self.raw_line[self.caret_column-1:self.finish_column]

    def __post_init__(self) -> None:
        super().__post_init__()
        if self.caret_column > self.finish_column:
            raise ValueError(f"Cannot initialize {self}. finish_column ({self.finish_column}) cannot be greater than caret_column ({self.caret_column})")
        if self.finish_column > len(self.raw_line):
            raise ValueError(f"Cannot initialize {self}. finish_column ({self.finish_column}) cannot be greater than raw_line's length ({len(self.raw_line)})")
        try:
            del self.other['locations'][0]['finish']['byte-column']
            del self.other['locations'][0]['finish']['column']
            del self.other['locations'][0]['finish']['display-column']
            del self.other['locations'][0]['finish']['file']
            del self.other['locations'][0]['finish']['line']
            if self.other['locations'][0]['finish'] == {}:
                del self.other['locations'][0]['finish']
            self._remove_locations_if_empty()
        except (IndexError, KeyError):
            pass

def parse_raw_gcc_dict(gcc_dict: Dict[Any, Any], path_prefix: Path) -> RawGccError:
    """
    >>> from pathlib import Path
    >>> path_prefix=Path('../src').resolve()
    >>> gcc_dict = {'children': [],
    ...             'column-origin': 1,
    ...             'kind': 'error',
    ...             'locations': [{'caret': {'byte-column': 9,
    ...                                     'column': 9,
    ...                                     'display-column': 9,
    ...                                     'file': './util/system.h',
    ...                                     'line': 333}}],
    ...             'message': '‘cs_args’ was not declared in this scope'}
    >>> print(_remove_path_prefix(parse_raw_gcc_dict(gcc_dict, path_prefix), path_prefix))
    RawGccError(kind='error', caret_column=9, file=PosixPath('util/system.h'), line=333, message='‘cs_args’ was not declared in this scope', raw_line='        LOCK(cs_args);\\n', other={'children': [], 'column-origin': 1})

    >>> gcc_dict = {'children': [],
    ...             'column-origin': 1,
    ...             'kind': 'error',
    ...             'locations': [{'caret': {'byte-column': 9,
    ...                                     'column': 9,
    ...                                     'display-column': 9,
    ...                                     'file': './util/system.h',
    ...                                     'line': 333},
    ...                             'finish': {'byte-column': 9,
    ...                                         'column': 9,
    ...                                         'display-column': 9,
    ...                                         'file': './util/system.h',
    ...                                         'line': 333}}],
    ...             'message': 'expected primary-expression before ‘decltype’'}
    >>> print(_remove_path_prefix(parse_raw_gcc_dict(gcc_dict, path_prefix), path_prefix))
    RawGccErrorRanged(kind='error', caret_column=9, file=PosixPath('util/system.h'), line=333, message='expected primary-expression before ‘decltype’', raw_line='        LOCK(cs_args);\\n', other={'children': [], 'column-origin': 1}, finish_column=9)

    >>> gcc_dict_bad_caret_columns = {'children': [],
    ...             'column-origin': 1,
    ...             'kind': 'error',
    ...             'locations': [{'caret': {'byte-column': 1,
    ...                                     'column': 2,
    ...                                     'display-column': 3,
    ...                                     'file': './util/system.h',
    ...                                     'line': 333}}],
    ...             'message': 'expected primary-expression before ‘decltype’'}
    >>> print(_remove_path_prefix(parse_raw_gcc_dict(gcc_dict_bad_caret_columns, path_prefix), path_prefix))
    Traceback (most recent call last):
    ...
    AssertionError: in 'caret', 'column' should be equal to 'display-column', {'children': [], 'column-origin': 1, 'kind': 'error', 'locations': [{'caret': {'byte-column': 1, 'column': 2, 'display-column': 3, 'file': './util/system.h', 'line': 333}}], 'message': 'expected primary-expression before ‘decltype’'}

    >>> gcc_dict = {'children': [],
    ...             'column-origin': 1,
    ...             'fixits': [{'next': {'byte-column': 91,
    ...                                  'column': 91,
    ...                                  'display-column': 91,
    ...                                  'file': 'rpc/blockchain.cpp',
    ...                                  'line': 270},
    ...                         'start': {'byte-column': 91,
    ...                                   'column': 91,
    ...                                   'display-column': 91,
    ...                                   'file': 'rpc/blockchain.cpp',
    ...                                   'line': 270},
    ...                         'string': ')'}],
    ...             'kind': 'error',
    ...             'locations': [{'caret': {'byte-column': 91,
    ...                                      'column': 91,
    ...                                      'display-column': 91,
    ...                                      'file': 'rpc/blockchain.cpp',
    ...                                      'line': 270}},
    ...                           {'caret': {'byte-column': 92,
    ...                                      'column': 92,
    ...                                      'display-column': 92,
    ...                                      'file': 'rpc/blockchain.cpp',
    ...                                      'line': 270},
    ...                            'finish': {'byte-column': 115,
    ...                                       'column': 115,
    ...                                       'display-column': 115,
    ...                                       'file': 'rpc/blockchain.cpp',
    ...                                       'line': 270}},
    ...                           {'caret': {'byte-column': 38,
    ...                                      'column': 38,
    ...                                      'display-column': 38,
    ...                                      'file': 'rpc/blockchain.cpp',
    ...                                      'line': 270}}],
    ...             'message': 'expected ‘)’ before ‘EXCLUSIVE_LOCKS_REQUIRED’'}
    >>> # TODO: prima era RawGccErrorRanged(kind='error', caret_column=92, file=PosixPath('rpc/blockchain.cpp'), line=270, message='expected ‘)’ before ‘EXCLUSIVE_LOCKS_REQUIRED’', raw_line='            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&block]() TS_ITCOIN_EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });', other={'children': [], 'column-origin': 1, 'fixits': [{'next': {'byte-column': 91, 'column': 91, 'display-column': 91, 'file': 'rpc/blockchain.cpp', 'line': 270}, 'start': {'byte-column': 91, 'column': 91, 'display-column': 91, 'file': 'rpc/blockchain.cpp', 'line': 270}, 'string': ')'}], 'locations': [{'caret': {'byte-column': 92, 'column': 92, 'display-column': 92, 'file': 'rpc/blockchain.cpp', 'line': 270}}, {'caret': {'byte-column': 38, 'column': 38, 'display-column': 38, 'file': 'rpc/blockchain.cpp', 'line': 270}}]}, finish_column=115)
    >>> print(_remove_path_prefix(parse_raw_gcc_dict(gcc_dict, path_prefix), path_prefix))
    RawGccErrorRanged(kind='error', caret_column=92, file=PosixPath('rpc/blockchain.cpp'), line=270, message='expected ‘)’ before ‘EXCLUSIVE_LOCKS_REQUIRED’', raw_line='            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&block]() TS_ITCOIN_EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });\\n', other={'children': [], 'column-origin': 1, 'fixits': [{'next': {'byte-column': 91, 'column': 91, 'display-column': 91, 'file': 'rpc/blockchain.cpp', 'line': 270}, 'start': {'byte-column': 91, 'column': 91, 'display-column': 91, 'file': 'rpc/blockchain.cpp', 'line': 270}, 'string': ')'}], 'locations': [{'caret': {'byte-column': 38, 'column': 38, 'display-column': 38, 'file': 'rpc/blockchain.cpp', 'line': 270}}]}, finish_column=115)

    >>> gcc_dict = {'kind': 'error',
    ...             'locations': [{'caret': {'byte-column': 13,
    ...                                      'column': 20,
    ...                                      'display-column': 20,
    ...                                      'file': '/usr/include/c++/11/condition_variable',
    ...                                      'line': 151},
    ...                            'finish': {'byte-column': 14,
    ...                                       'column': 21,
    ...                                       'display-column': 21,
    ...                                       'file': '/usr/include/c++/11/condition_variable',
    ...                                       'line': 151}}],
    ...             'message': 'some message'}
    >>> print(parse_raw_gcc_dict(gcc_dict, path_prefix))
    RawGccErrorRanged(kind='error', caret_column=13, file=PosixPath('/usr/include/c++/11/condition_variable'), line=151, message='some message', raw_line='\\twhile (!__p())\\n', other={}, finish_column=14)

    >>> gcc_dict = {'kind': 'error',
    ...             'locations': [{'caret': {'byte-column': 13,
    ...                                      'column': 20,
    ...                                      'display-column': 20,
    ...                                      'file': 'rpc/blockchain.cpp',
    ...                                      'line': 151},
    ...                            'finish': {'byte-column': 14,
    ...                                       'column': 21,
    ...                                       'display-column': 21,
    ...                                       'file': 'rpc/blockchain.cpp',
    ...                                       'line': 151}}],
    ...             'message': 'some message'}
    >>> print(_remove_path_prefix(parse_raw_gcc_dict(gcc_dict, path_prefix), path_prefix))
    Traceback (most recent call last):
    ...
    ValueError: in 'caret', column can be different from byte-column only if raw_line contains a tab, {'kind': 'error', 'locations': [{'caret': {'byte-column': 13, 'column': 20, 'display-column': 20, 'file': 'rpc/blockchain.cpp', 'line': 151}, 'finish': {'byte-column': 14, 'column': 21, 'display-column': 21, 'file': 'rpc/blockchain.cpp', 'line': 151}}], 'message': 'some message'}

    >>> gcc_dict = {'kind': 'error',
    ...             'locations': [{'caret': {'byte-column': 20,
    ...                                      'column': 20,
    ...                                      'display-column': 20,
    ...                                      'file': 'rpc/blockchain.cpp',
    ...                                      'line': 151},
    ...                            'finish': {'byte-column': 14,
    ...                                       'column': 21,
    ...                                       'display-column': 21,
    ...                                       'file': 'rpc/blockchain.cpp',
    ...                                       'line': 151}}],
    ...             'message': 'some message'}
    >>> print(_remove_path_prefix(parse_raw_gcc_dict(gcc_dict, path_prefix), path_prefix))
    Traceback (most recent call last):
    ...
    ValueError: in 'finish', column can be different from byte-column only if raw_line contains a tab, {'kind': 'error', 'locations': [{'caret': {'byte-column': 20, 'column': 20, 'display-column': 20, 'file': 'rpc/blockchain.cpp', 'line': 151}, 'finish': {'byte-column': 14, 'column': 21, 'display-column': 21, 'file': 'rpc/blockchain.cpp', 'line': 151}}], 'message': 'some message'}

    Specific test cases for the gcc 10 compatibility
    >>> gcc10_dict = {'children': [],
    ...               'kind': 'error',
    ...               'locations': [{'caret': {'column': 37, 'file': 'netbase.cpp', 'line': 34},
    ...                              'finish': {'column': 46, 'file': 'netbase.cpp', 'line': 34}}],
    ...               'message': 'expected initializer before ‘GUARDED_BY’'}
    >>> print(_remove_path_prefix(parse_raw_gcc_dict(gcc10_dict, path_prefix), path_prefix))
    Traceback (most recent call last):
    ...
    ValueError: caret does not contain the 'display-column' field. Are you running on gcc 10? Only gcc 11 is supported: {'children': [], 'kind': 'error', 'locations': [{'caret': {'column': 37, 'file': 'netbase.cpp', 'line': 34}, 'finish': {'column': 46, 'file': 'netbase.cpp', 'line': 34}}], 'message': 'expected initializer before ‘GUARDED_BY’'}
    """
    kind = gcc_dict['kind']
    message = gcc_dict['message']

    finish = None
    try:
        locations = gcc_dict['locations']
        if len(locations) == 1:
            caret = locations[0]['caret']
            if 'finish' in locations[0]:
                finish = locations[0]['finish']
        if len(locations) in [2, 3]:
            # TODO: in realtà qui sarebbe intelligente assicurarci che in
            #       locations esista uno ed un solo item che abbia come chiavi
            #       sia caret che finish, e prendere quello.
            try:
                caret = locations[1]['caret']
                finish = locations[1]['finish']
            except KeyError as e:
                raise ValueError(f"locations has {len(locations)} elements, and I only know how to deal with a very specific case. {gcc_dict}") from e
        assert len(locations) in [1, 2, 3], f"locations should be a list of 1 element, or a list of 2 or 3 elements with a specific structure. {gcc_dict}"
    except Exception as e:
        pprint(gcc_dict)
        raise

    # gcc 10 messages do not contain the 'display-column' field.
    # We do not know how to handle it.
    if 'display-column' not in caret:
        raise ValueError(f"caret does not contain the 'display-column' field. Are you running on gcc 10? Only gcc 11 is supported: {gcc_dict}")

    # display-column is not present in gcc 10
    #
    # If it is there, let's require that 'column' and 'display-column' have the
    # same value. This will ensure we are not dealing with a utf-8 encoded
    # source code, for example, and we do not want to think about how to manage
    # it.
    assert caret['column'] == caret['display-column'], f"in 'caret', 'column' should be equal to 'display-column', {gcc_dict}"

    if finish is not None:
        # if finish is defined, do the same check on it
        assert finish['column'] == finish['display-column'], f"in 'finish', 'column' should be equal to 'display-column', {gcc_dict}"
        # ensure the start and end positions pertain to the same line in the
        # same file
        assert caret['file'] == finish['file'], f"caret.file should be equal to finish.file. {gcc_dict}"
        assert caret['line'] == finish['line'], f"caret.line should be equal to finish.line. {gcc_dict}"

    f_name = caret['file']
    line = caret['line']

    f_path = (path_prefix / f_name).resolve()
    raw_line = linecache.getline(str(f_path), line)
    if raw_line == '':
        raise FileNotFoundError(f"Can't open line {line} of {f_path}")

    if (caret['column'] != caret['byte-column']) and ('\t' not in raw_line):
        # NOTE: byte-column is not present in gcc 10.
        #
        # If there is a tab in the source code, column and byte-column will have
        # different values (because of tab expansion), and we will care about
        # byte-column.
        #
        # TODO: this condition should be stricter, because we do not know if
        #       column and byte-column are differing for additional reasons.
        raise ValueError(f"in 'caret', column can be different from byte-column only if raw_line contains a tab, {gcc_dict}")

    if (finish is not None) and (finish['column'] != finish['byte-column']) and ('\t' not in raw_line):
        # NOTE: byte-column is not present in gcc 10.
        #
        # If there is a tab in the source code, column and byte-column will have
        # different values (because of tab expansion), and we will care about
        # byte-column.
        #
        # TODO: this condition should be stricter, because we do not know if
        #       column and byte-column are differing for additional reasons.
        raise ValueError(f"in 'finish', column can be different from byte-column only if raw_line contains a tab, {gcc_dict}")

    common_data = (
        kind,
        # byte-column is not present in gcc 10
        caret['byte-column'],
        f_path,
        line,
        message,
        raw_line,
        gcc_dict,
    )
    if finish is not None:
        return RawGccErrorRanged(
            *common_data,
            # byte-column is not present in gcc 10
            finish['byte-column'],
        )
    return RawGccError(*common_data)

@dataclass
class RenameSpec:
    """
    >>> RenameSpec("GUARDED_BY", 7, 6, "xGUARDED_BYx")
    Traceback (most recent call last):
    ...
    ValueError: start_pos (7) should be less or equal than end_pos (6)

    >>> RenameSpec("GUARDED_BY", 2, 2, "xxxxxxxxxx")
    Traceback (most recent call last):
    ...
    ValueError: Cannot build RenameSpec(symbol='GUARDED_BY', start_pos=2, end_pos=2, raw_line='xxxxxxxxxx'). The difference between end_pos and start_pos should be 9. It is 0.

    >>> RenameSpec("GUARDED_BY", 2, 11, "xxx")
    Traceback (most recent call last):
    ...
    ValueError: Cannot build RenameSpec(symbol='GUARDED_BY', start_pos=2, end_pos=11, raw_line='xxx'). symbol can't be longer than raw_line

    >>> RenameSpec("GUARDED_BY", 2, 11, "GUARDED_BY")
    Traceback (most recent call last):
    ...
    ValueError: Cannot build RenameSpec(symbol='GUARDED_BY', start_pos=2, end_pos=11, raw_line='GUARDED_BY'). end_pos goes past raw_line length (10)

    >>> RenameSpec("GUARDED_BY", 5, 14, "pre_GUARDED_BY_post")
    RenameSpec(symbol='GUARDED_BY', start_pos=5, end_pos=14, raw_line='pre_GUARDED_BY_post')
    """
    symbol: str
    start_pos: int
    end_pos: int
    raw_line: str

    def __post_init__(self):
        # end_pos and start_pos come from the structured gcc error message.
        # The difference between those values must be 1 less than the lenght of
        # the problematic symbol (this is a gcc decision: we simply enforce this
        # invariant here).
        #
        # TODO: this means that for a 1-charachter symbol gcc would probably
        #       spit out the values for "start_pos" and "end_pos". This should
        #       be verified with a test on a fake C++ program.
        pos_delta = self.end_pos - self.start_pos

        if pos_delta < 0:
            raise ValueError(f"start_pos ({self.start_pos}) should be less or equal than end_pos ({self.end_pos})")

        # ensure the length of the symbol as identified by the compiler is the
        # same as the one we identified looking at the error message via a regex
        if len(self.symbol) != (pos_delta + 1):
            raise ValueError(f"Cannot build {self}. The difference between end_pos and start_pos should be { len(self.symbol) - 1}. It is {pos_delta}.")

        raw_line_length = len(self.raw_line)

        if len(self.symbol) > raw_line_length:
            raise ValueError(f"Cannot build {self}. symbol can't be longer than raw_line")

        if self.end_pos > raw_line_length:
            raise ValueError(f"Cannot build {self}. end_pos goes past raw_line length ({raw_line_length})")

        if self.raw_line[(self.start_pos - 1):self.end_pos] != self.symbol:
            raise ValueError(f"Cannot build {self}. The symbol '{self.symbol}' should be contained in raw_line at positions [{self.start_pos - 1}:{self.end_pos}]. Got: '{self.raw_line[self.start_pos - 1:self.end_pos]}'")

    def get_prefixed(self, prefix: str) -> str:
        """
        >>> RenameSpec("ABC", 3, 5, "--ABC--").get_prefixed("PREFIX_")
        '--PREFIX_ABC--'
        """
        tk_start  = self.raw_line[:self.start_pos-1]
        tk_symbol = self.raw_line[self.start_pos-1:self.end_pos]
        tk_end    = self.raw_line[self.end_pos:]
        return tk_start + prefix + tk_symbol + tk_end

@dataclass
class TsError:
    """
    A structured object suited for representing the kind of errors we are
    interested in.
    """
    file_name: Path
    line: int
    spec: RenameSpec

def _remove_path_prefix(record: Union[RawGccError, TsError], prefix: Path):
    if isinstance(record, RawGccError):
        return dataclasses.replace(record, file=record.file.relative_to(prefix))
    if isinstance(record, TsError):
        return dataclasses.replace(record, file_name=record.file_name.relative_to(prefix))
    raise ValueError(f"Unsupported type {type(record)} for {record}")

RenamingPlan = Dict[Path, Dict[int, RenameSpec]]

def find_symbol(message: str, exp: str, allowed_symbols: Set[str]) -> Optional[str]:
    """
    >>> print(find_symbol("this is a line", r"this (i.) a line", ["ia"]))
    None

    >>> print(find_symbol("this blip a line", r"this (i.) a line", ["blip"]))
    None

    >>> print(find_symbol("this is a line", r"this (is) a line", ["ia", "is"]))
    is

    >>> find_symbol("this is a line", r"this", ['line'])
    Traceback (most recent call last):
    ...
    ValueError: The passed regular expression 'this' does not have any matching groups
    """
    if (match := re.search(exp, message)) is None:
        return None
    if len(match.groups()) == 0:
        raise ValueError(f"The passed regular expression '{exp}' does not have any matching groups")
    if (symbol := match.group(1)) in allowed_symbols:
        return symbol
    return None

def find_symbol_2(message: str, exp: str, allowed_symbols: Set[str]) -> Optional[Tuple[str, str]]:
    """
    >>> print(find_symbol_2("variable ‘LOCKABLE Epoch’ has initializer but incomplete type", r"variable ‘([^’]*) ([^’]*)’ has initializer but incomplete type", {'LOCKABLE'}))
    ('LOCKABLE', 'Epoch')

    >>> print(find_symbol_2("variable ‘BLURB Epoch’ has initializer but incomplete type", r"variable ‘([^’]*) ([^’]*)’ has initializer but incomplete type", {'LOCKABLE'}))
    None

    >>> print(find_symbol_2("variable ‘BLURB Epoch’ has initializer but incomplete type", r"variable ‘([^’]*) ([^’]*)’ has initializer but incomplete type", {'LOCKABLE', 'BLURB'}))
    ('BLURB', 'Epoch')
    """
    if (match := re.search(exp, message)) is None:
        return None
    if len(match.groups()) != 2:
        raise ValueError(f"The passed regular expression '{exp}' must have exactly 2 matching groups")
    if (symbol := match.group(1)) in allowed_symbols:
        return (symbol, match.group(2))
    return None

def find_symbol_not_a_type_with_fixit(message: str, allowed_symbols: Set[str], prefix: str) -> Optional[str]:
    """
    >>> print(find_symbol_not_a_type_with_fixit("‘NO_THREAD_SAFETY_ANALYSIS’ does not name a type; did you mean ‘TS_ITCOIN_NO_THREAD_SAFETY_ANALYSIS’?", {'NO_THREAD_SAFETY_ANALYSIS', 'GINO'}, "TS_ITCOIN_"))
    NO_THREAD_SAFETY_ANALYSIS

    >>> print(find_symbol_not_a_type_with_fixit("‘GINO’ does not name a type; did you mean ‘TS_ITCOIN_GINO’?", {'NO_THREAD_SAFETY_ANALYSIS', 'GINO'}, "TS_ITCOIN_"))
    GINO

    >>> print(find_symbol_not_a_type_with_fixit("‘WRONG’ does not name a type; did you mean ‘TS_ITCOIN_WRONG’?", {'NO_THREAD_SAFETY_ANALYSIS', 'GINO'}, "TS_ITCOIN_"))
    None

    >>> print(find_symbol_not_a_type_with_fixit("‘GINO’ does not name a type; did you mean ‘TS_WRONGPREFIX_GINO’?", {'NO_THREAD_SAFETY_ANALYSIS', 'GINO'}, "TS_ITCOIN_"))
    None
    """
    symbols = "|".join(allowed_symbols)
    prefixed_symbols = "|".join([(prefix + symbol) for symbol in allowed_symbols])
    exp = fr"‘({symbols})’ does not name a type; did you mean ‘({prefixed_symbols})’\?"
    if (match := re.search(exp, message)) is None:
        return None
    if (symbol := match.group(1)) in allowed_symbols:
        return symbol
    return None

def ts_error_from_gcc_error(gcc_error: RawGccErrorRanged, allowed_symbols: Set[str]) -> Optional[TsError]:
    """
    >>> from pathlib import Path
    >>> path_prefix=Path('../src').resolve()
    >>> fake_locations = { 'locations': [{
    ... 'caret': {
    ...     'byte-column': 47,
    ...     'column': 47,
    ...     'display-column': 47,
    ...     'file': './util/system.h',
    ...     'line': 187
    ... },
    ... 'finish': {
    ...     'byte-column': 66,
    ...     'column': 66,
    ...     'display-column': 66,
    ...     'file': './util/system.h',
    ...     'line': 187
    ... }
    ... }]}
    >>> msg = {
    ... "kind": "error",
    ... "message": "CIAO",
    ... } | fake_locations
    >>> print(ts_error_from_gcc_error(parse_raw_gcc_dict(msg, path_prefix), {'SCOPED_LOCKABLE'}))
    None

    >>> msg = {
    ... "kind": "error",
    ... "message": "ISO C++ forbids declaration of ‘CIAO’ with no type",
    ... } | fake_locations
    >>> print(ts_error_from_gcc_error(parse_raw_gcc_dict(msg, path_prefix), {'SCOPED_LOCKABLE'}))
    None

    >>> msg = {
    ... "kind": "error",
    ... "message": "ISO C++ forbids declaration of ‘TS_ITCOIN_GUARDED_BY’ with no type",
    ... } | fake_locations
    >>> print(_remove_path_prefix(ts_error_from_gcc_error(parse_raw_gcc_dict(msg, path_prefix), {'TS_ITCOIN_GUARDED_BY'}), path_prefix))
    TsError(file_name=PosixPath('util/system.h'), line=187, spec=RenameSpec(symbol='TS_ITCOIN_GUARDED_BY', start_pos=47, end_pos=66, raw_line='    std::set<std::string> m_network_only_args TS_ITCOIN_GUARDED_BY(cs_args);\\n'))

    >>> msg = {
    ... "kind": "error",
    ... "message": "expected initializer before ‘TS_ITCOIN_GUARDED_BY’",
    ... } | fake_locations
    >>> print(_remove_path_prefix(ts_error_from_gcc_error(parse_raw_gcc_dict(msg, path_prefix), {'TS_ITCOIN_GUARDED_BY'}), path_prefix))
    TsError(file_name=PosixPath('util/system.h'), line=187, spec=RenameSpec(symbol='TS_ITCOIN_GUARDED_BY', start_pos=47, end_pos=66, raw_line='    std::set<std::string> m_network_only_args TS_ITCOIN_GUARDED_BY(cs_args);\\n'))
    """
    if gcc_error.kind != 'error':
        return None
    expressions = [
        r"ISO C\+\+ forbids declaration of ‘([^’]*)’ with no type",
        r"expected initializer before ‘([^’]*)’",
        r"expected ‘{’ before ‘([^’]*)’",
        r"expected ‘,’ or ‘;’ before ‘([^’]*)’",
    ]

    symbols = list(map(lambda exp: find_symbol(gcc_error.message, exp, allowed_symbols), expressions))
    symbol = next(filter(lambda sym: sym is not None, symbols), None)
    if symbol is None:
        return None

    # the symbol is among the one we are lookign for
    return TsError(
        gcc_error.file,
        gcc_error.line,
        RenameSpec(symbol, gcc_error.caret_column, gcc_error.finish_column, gcc_error.raw_line)
    )

def ts_error_from_gcc_error_not_a_type_with_fixit(gcc_error: RawGccErrorRanged, threadsafety_symbols: Set[str], prefix: str) -> Optional[TsError]:
    if gcc_error.kind != 'error':
        return None

    symbol = find_symbol_not_a_type_with_fixit(gcc_error.message, threadsafety_symbols, prefix)
    if symbol is None:
        return None

    # the symbol is among the one we are lookign for
    return TsError(
        gcc_error.file,
        gcc_error.line,
        RenameSpec(symbol, gcc_error.caret_column, gcc_error.finish_column, gcc_error.raw_line)
    )

def ts_error_from_gcc_error_raw_line(gcc_error: RawGccError, allowed_symbols: Set[str]) -> Optional[TsError]:
    """
    """
    if gcc_error.kind != 'error':
        return None

    if gcc_error.message != "expected initializer before ‘:’ token":
        return None

    exp = r"class[ ]+([^ :]+)[ ]+"
    symbol = find_symbol(gcc_error.raw_line, exp, allowed_symbols)
    if symbol is None:
        return None

    # the symbol is among the one we are lookign for
    start_pos = gcc_error.raw_line.find(symbol) + 1
    end_pos = start_pos + len(symbol) - 1
    return TsError(
        gcc_error.file,
        gcc_error.line,
        RenameSpec(symbol, start_pos, end_pos, gcc_error.raw_line)
    )

def ts_error_from_gcc_error_raw_line_and_message(gcc_error: RawGccErrorRanged, allowed_symbols: Set[str]) -> Optional[TsError]:
    """
    >>> e = RawGccErrorRanged('error', 16, Path('util/epochguard.h'), 33, "variable ‘LOCKABLE Epoch’ has initializer but incomplete type", "class LOCKABLE Epoch", {}, 20)
    >>> print(ts_error_from_gcc_error_raw_line_and_message(e, {'LOCKABLE'}))
    TsError(file_name=PosixPath('util/epochguard.h'), line=33, spec=RenameSpec(symbol='LOCKABLE', start_pos=7, end_pos=14, raw_line='class LOCKABLE Epoch'))
    """
    if gcc_error.kind != 'error':
        return None

    exp = r"variable ‘([^’]*) ([^’]*)’ has initializer but incomplete type"

    sym_and_kw = find_symbol_2(gcc_error.message, exp, allowed_symbols)
    if sym_and_kw is None:
        return None

    (symbol, keyword) = sym_and_kw

    # verify that gcc_error start-end point to the keyword (not the symbol!)
    if gcc_error.highlighted_string() != keyword:
        raise ValueError(f"Error matching {gcc_error.message}. The second token should be '{keyword}'. It is '{gcc_error.highlighted_string()}'")

    # verify that the raw line contains the concatenation of symbol and keyword,
    # limiting our search to the position identified by gcc
    if gcc_error.raw_line.endswith(f"{symbol} {keyword}", 0, gcc_error.finish_column) == False:
        raise ValueError(f"'{symbol} {keyword}' should be contained in {gcc_error.raw_line[:gcc_error.finish_column]}")

    # the symbol is among the ones we are lookign for
    start_pos = gcc_error.finish_column - len(keyword) - len(symbol)
    end_pos = start_pos + len(symbol) - 1
    return TsError(
        gcc_error.file,
        gcc_error.line,
        RenameSpec(symbol, start_pos, end_pos, gcc_error.raw_line)
    )

def parse_gcc_output(gcc_stderr: str) -> Tuple[List[str], List[Dict[Any, Any]]]:
    """
    stderr is a mix between json output and string messages.
    String messages are returned in the first return value.
    The second return value is a list of dictionaries, each containing an error
    message.

    NOTE: gcc error messages in json are a 2-deep list. This function flattens
          them.

    >>> txt = '''
    ... []
    ... [{"kind": "error", "children": [], "locations": [{"finish": {"line": 16, "file": "warnings.cpp", "column": 47}, "caret": {"line": 16, "file": "warnings.cpp", "column": 38}}], "message": "expected initializer before ‘GUARDED_BY’"}, {"kind": "error", "children": [], "locations": [{"finish": {"line": 17, "file": "warnings.cpp", "column": 42}, "caret": {"line": 17, "file": "warnings.cpp", "column": 33}}], "message": "expected initializer before ‘GUARDED_BY’"}, {"kind": "error", "children": [], "locations": [{"finish": {"line": 18, "file": "warnings.cpp", "column": 50}, "caret": {"line": 18, "file": "warnings.cpp", "column": 41}}], "message": "expected initializer before ‘GUARDED_BY’"}, {"kind": "error", "children": [], "locations": [{"finish": {"line": 23, "file": "warnings.cpp", "column": 19}, "caret": {"line": 23, "file": "warnings.cpp", "column": 5}}], "message": "‘g_misc_warnings’ was not declared in this scope"}, {"kind": "error", "fixits": [{"next": {"line": 29, "file": "warnings.cpp", "column": 24}, "string": "GetfLargeWorkForkFound", "start": {"line": 29, "file": "warnings.cpp", "column": 5}}], "children": [], "locations": [{"finish": {"line": 29, "file": "warnings.cpp", "column": 23}, "caret": {"line": 29, "file": "warnings.cpp", "column": 5}}], "message": "‘fLargeWorkForkFound’ was not declared in this scope; did you mean ‘GetfLargeWorkForkFound’?"}, {"kind": "error", "fixits": [{"next": {"line": 35, "file": "warnings.cpp", "column": 31}, "string": "GetfLargeWorkForkFound", "start": {"line": 35, "file": "warnings.cpp", "column": 12}}], "children": [], "locations": [{"finish": {"line": 35, "file": "warnings.cpp", "column": 30}, "caret": {"line": 35, "file": "warnings.cpp", "column": 12}}], "message": "‘fLargeWorkForkFound’ was not declared in this scope; did you mean ‘GetfLargeWorkForkFound’?"}, {"kind": "error", "fixits": [{"next": {"line": 41, "file": "warnings.cpp", "column": 32}, "string": "SetfLargeWorkInvalidChainFound", "start": {"line": 41, "file": "warnings.cpp", "column": 5}}], "children": [], "locations": [{"finish": {"line": 41, "file": "warnings.cpp", "column": 31}, "caret": {"line": 41, "file": "warnings.cpp", "column": 5}}], "message": "‘fLargeWorkInvalidChainFound’ was not declared in this scope; did you mean ‘SetfLargeWorkInvalidChainFound’?"}, {"kind": "error", "children": [], "locations": [{"finish": {"line": 58, "file": "warnings.cpp", "column": 24}, "caret": {"line": 58, "file": "warnings.cpp", "column": 10}}], "message": "‘g_misc_warnings’ was not declared in this scope"}, {"kind": "error", "fixits": [{"next": {"line": 63, "file": "warnings.cpp", "column": 28}, "string": "GetfLargeWorkForkFound", "start": {"line": 63, "file": "warnings.cpp", "column": 9}}], "children": [], "locations": [{"finish": {"line": 63, "file": "warnings.cpp", "column": 27}, "caret": {"line": 63, "file": "warnings.cpp", "column": 9}}], "message": "‘fLargeWorkForkFound’ was not declared in this scope; did you mean ‘GetfLargeWorkForkFound’?"}, {"kind": "error", "fixits": [{"next": {"line": 66, "file": "warnings.cpp", "column": 43}, "string": "SetfLargeWorkInvalidChainFound", "start": {"line": 66, "file": "warnings.cpp", "column": 16}}], "children": [], "locations": [{"finish": {"line": 66, "file": "warnings.cpp", "column": 42}, "caret": {"line": 66, "file": "warnings.cpp", "column": 16}}], "message": "‘fLargeWorkInvalidChainFound’ was not declared in this scope; did you mean ‘SetfLargeWorkInvalidChainFound’?"}]
    ... make[2]: *** [Makefile:11609: libbitcoin_common_a-warnings.o] Error 1
    ... make[2]: *** Waiting for unfinished jobs....
    ... []
    ... [{"kind": "error", "children": [], "locations": [{"finish": {"line": 271, "file": "wallet/walletdb.cpp", "column": 143}, "caret": {"line": 271, "file": "wallet/walletdb.cpp", "column": 120}}], "message": "expected initializer before ‘EXCLUSIVE_LOCKS_REQUIRED’"}, {"kind": "error", "children": [{"kind": "note", "locations": [{"finish": {"line": 680, "file": "wallet/walletdb.cpp", "column": 98}, "caret": {"line": 680, "file": "wallet/walletdb.cpp", "column": 92}, "start": {"line": 680, "file": "wallet/walletdb.cpp", "column": 79}}], "message": "in passing argument 4 of ‘bool ReadKeyValue(CWallet*, CDataStream&, CDataStream&, std::string&, std::string&, const KeyFilterFn&)’"}], "locations": [{"finish": {"line": 684, "file": "wallet/walletdb.cpp", "column": 58}, "caret": {"line": 684, "file": "wallet/walletdb.cpp", "column": 50}}], "message": "invalid initialization of reference of type ‘std::string&’ {aka ‘std::__cxx11::basic_string<char>&’} from expression of type ‘CWalletScanState’"}, {"kind": "error", "children": [{"kind": "note", "locations": [{"finish": {"line": 680, "file": "wallet/walletdb.cpp", "column": 98}, "caret": {"line": 680, "file": "wallet/walletdb.cpp", "column": 92}, "start": {"line": 680, "file": "wallet/walletdb.cpp", "column": 79}}], "message": "in passing argument 4 of ‘bool ReadKeyValue(CWallet*, CDataStream&, CDataStream&, std::string&, std::string&, const KeyFilterFn&)’"}], "locations": [{"finish": {"line": 734, "file": "wallet/walletdb.cpp", "column": 58}, "caret": {"line": 734, "file": "wallet/walletdb.cpp", "column": 56}}], "message": "invalid initialization of reference of type ‘std::string&’ {aka ‘std::__cxx11::basic_string<char>&’} from expression of type ‘CWalletScanState’"}]
    ... make[2]: *** [Makefile:13177: wallet/libbitcoin_wallet_a-walletdb.o] Error 1
    ... make[1]: *** [Makefile:19500: all-recursive] Error 1
    ... make: *** [Makefile:807: all-recursive] Error 1
    ... '''
    >>> string_messages = ""
    >>> structured_messages = ""
    >>> (string_messages, structured_messages) = parse_gcc_output(txt)
    >>> for msg in string_messages:
    ...     print(msg)
    make[2]: *** [Makefile:11609: libbitcoin_common_a-warnings.o] Error 1
    make[2]: *** Waiting for unfinished jobs....
    make[2]: *** [Makefile:13177: wallet/libbitcoin_wallet_a-walletdb.o] Error 1
    make[1]: *** [Makefile:19500: all-recursive] Error 1
    make: *** [Makefile:807: all-recursive] Error 1

    Non testo il contenuto dei messaggi strutturati perché divento pazzo, solo
    un po' la struttura.

    >>> print(len(structured_messages))
    13

    >>> for msg in structured_messages:
    ...     assert('kind' in msg)
    ...     assert('locations' in msg)
    ...     assert(len(msg['locations']) > 0)
    """
    string_messages: List[str] = []
    structured_messages: List[Dict[Any, Any]] = []

    for line in gcc_stderr.split('\n'):
        if line == '':
            continue
        try:
            decoded = json.loads(line)
            if type(decoded) is list:
                for msg in decoded:
                    structured_messages.append(msg)
            else:
                structured_messages.append(decoded)
        except json.decoder.JSONDecodeError:
            decoded = line
            string_messages.append(line)

    return (string_messages, structured_messages)

def extract_threadsafety_symbols(threadsafety_h_contents: str, macro_prefix: str) -> Set[str]:
    """
    >>> threadsafety_h_contents = Path('../src/threadsafety.h').read_text()
    >>> symbols = extract_threadsafety_symbols(threadsafety_h_contents, 'TS_ITCOIN_')
    >>> for symbol in sorted(symbols):
    ...     print(symbol)
    ACQUIRED_AFTER
    ACQUIRED_BEFORE
    ASSERT_EXCLUSIVE_LOCK
    EXCLUSIVE_LOCKS_REQUIRED
    EXCLUSIVE_LOCK_FUNCTION
    EXCLUSIVE_TRYLOCK_FUNCTION
    GUARDED_BY
    LOCKABLE
    LOCKS_EXCLUDED
    LOCK_RETURNED
    NO_THREAD_SAFETY_ANALYSIS
    PT_GUARDED_BY
    SCOPED_LOCKABLE
    SHARED_LOCKS_REQUIRED
    SHARED_LOCK_FUNCTION
    SHARED_TRYLOCK_FUNCTION
    UNLOCK_FUNCTION
    """
    exp = fr"^#define {macro_prefix}([^ \(]*)"
    symbols = set()
    for line in threadsafety_h_contents.split('\n'):
        match = re.search(exp, line)
        if match is None:
            continue
        if len(match.groups()) > 1:
            raise ValueError("there can be only 0 or 1 matches")
        symbol = match.group(1)
        symbols.add(symbol)

    return symbols

def try_build_project(makefile_dir: Path, jobs: int, load_average: int) -> Tuple[int, str]:
    logger.debug("Starting build process...")
    makefile_dir = makefile_dir.resolve(strict=True)
    result = subprocess.run(
        [
            'make',
            f'--directory={makefile_dir}',
            f'--jobs={jobs}',
            f'--load-average={load_average}',
        ],
        capture_output=True,
        encoding='UTF-8',
        text=True,
    )
    logger.debug(f"Build process ended with return code {result.returncode}")

    return (result.returncode, result.stderr)

def build_renaming_plan(ts_errors: List[TsError], path_prefix: Path) -> RenamingPlan:
    """
    >>> ts_errors = [
    ...     TsError(Path('/file_a'), 1, RenameSpec('LOCKS_EXCLUDED', 10, 23, '12345678_LOCKS_EXCLUDED')),
    ...     TsError(Path('/file_a'), 1, RenameSpec('LOCKS_EXCLUDED', 10, 23, '12345678_LOCKS_EXCLUDED')),
    ...     TsError(Path('/file_a'), 2, RenameSpec('GUARDED_BY',     10, 19, '12345678_GUARDED_BY')),
    ...     TsError(Path('/file_b'), 1, RenameSpec('LOCKS_EXCLUDED', 10, 23, '12345678_LOCKS_EXCLUDED')),
    ... ]
    >>> pprint(build_renaming_plan(ts_errors, Path('/')))
    {PosixPath('/file_a'): {1: RenameSpec(symbol='LOCKS_EXCLUDED', start_pos=10, end_pos=23, raw_line='12345678_LOCKS_EXCLUDED'),
                            2: RenameSpec(symbol='GUARDED_BY', start_pos=10, end_pos=19, raw_line='12345678_GUARDED_BY')},
     PosixPath('/file_b'): {1: RenameSpec(symbol='LOCKS_EXCLUDED', start_pos=10, end_pos=23, raw_line='12345678_LOCKS_EXCLUDED')}}
    """
    logger = logging.getLogger(__name__)
    renaming_plan: RenamingPlan = {}
    for tse in ts_errors:
        if (
            tse.file_name in renaming_plan and
            tse.line in renaming_plan[tse.file_name]
        ):
            # we have already scheduled a replace operation for
            # renaming_plan[tse.file_name][tse.line]. Let's ignore everything
            # else. We'll find this error at the next make iteration.
            logger.debug(f"At {tse.file_name.relative_to(path_prefix)}:{tse.line}, not executing {tse.spec}. We already have a renaming plan for that line")
            continue
        if tse.file_name not in renaming_plan:
            renaming_plan[tse.file_name] = {}
        renaming_plan[tse.file_name] |= { tse.line: tse.spec }

    return renaming_plan

def prefix_symbols_in_file(file_contents: str, specs: Dict[int, RenameSpec], prefix: str) -> str:
    """
    >>> f = ("\\n"
    ... "hello please modify me\\n"
    ... "also change here\\n")
    >>> print(f)
    <BLANKLINE>
    hello please modify me
    also change here
    <BLANKLINE>

    >>> specs = {
    ...     2: RenameSpec("modify", 14, 19, "h---o p----e modify me\\n"),
    ...     3: RenameSpec("change", 6, 11, "a--o change h--e"),
    ... }
    >>> print(prefix_symbols_in_file(f, specs, "DON'T_"))
    Traceback (most recent call last):
    ...
    RuntimeError: These two values should be the same: line='hello please modify me\\n', spec.raw_line='h---o p----e modify me\\n'

    >>> specs = {
    ...     2: RenameSpec("modify", 14, 19, "hello please modify me\\n"),
    ...     3: RenameSpec("change", 6, 11, "also change here\\n"),
    ... }
    >>> print(prefix_symbols_in_file(f, specs, "DON'T_"))
    <BLANKLINE>
    hello please DON'T_modify me
    also DON'T_change here
    <BLANKLINE>

    >>> specs = {
    ...     50: RenameSpec("modify", 10, 15, "12345678 modify me"),
    ... }
    >>> print(prefix_symbols_in_file(f, specs, "DON'T_"))
    Traceback (most recent call last):
    ...
    RuntimeError: The following RenameSpecs were not consumed: {50: RenameSpec(symbol='modify', start_pos=10, end_pos=15, raw_line='12345678 modify me')}

    Empty lines at start of file are kept. Empty lines at end of file are
    removed. This is bad, but not a big problem.
    >>> specs = {}
    >>> print(prefix_symbols_in_file("\\n" + f, specs, "DON'T_"))
    <BLANKLINE>
    <BLANKLINE>
    hello please modify me
    also change here
    <BLANKLINE>
    """
    # specs is going to be modified. Let's do a deep copy.
    wrk_specs: Dict[int, RenameSpec] = copy.deepcopy(specs)
    modified_lines: List[str] = []
    for line_no, line in enumerate(file_contents.splitlines(keepends=True), start=1):
        if line_no not in wrk_specs:
            modified_lines.append(line)
            continue
        spec = wrk_specs[line_no]
        if line != spec.raw_line:
            raise RuntimeError(f'These two values should be the same: {line=}, {spec.raw_line=}')
        modified_lines.append(spec.get_prefixed(prefix))
        del wrk_specs[line_no]

    if len(wrk_specs) > 0:
        raise RuntimeError(f"The following RenameSpecs were not consumed: {wrk_specs}")

    return ''.join(modified_lines)

def parse_cmdline() -> argparse.Namespace:
    # Return the set of CPUs the current process is restricted to
    available_cpu_cores = len(os.sched_getaffinity(0))

    script_dir = Path(__file__).parent.resolve()
    default_path_prefix = (script_dir / '../src').resolve()
    default_makefile_directory = (script_dir / '..').resolve()

    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter)
    subparsers = parser.add_subparsers(help='Reconfigure or run the build. Default: run', dest="subcommand")

    # create the parser for the "reconfigure" subcommand
    parser_reconfigure = subparsers.add_parser('reconfigure', help='Run the project configuration script, modifying the environment so that gcc generates its output in json')
    parser_reconfigure.add_argument(
        'configure_script',
        nargs='?',
        type=Path,
        metavar='CONFIGURE_SCRIPT',
        default=Path('configure-itcoin-core-dev.sh'),
        help='Confifuration script to be run. It will be run setting CXXFLAGS="-fsdss" so that gcc will generate its output in json (default: configure-itcoin-core-dev.sh)'
    )

    # create the parser for the "run" subcommand
    parser_run = subparsers.add_parser('run', help='run the build as many times as needed, renaming symbols until the build succeedsd.')

    parser_run.add_argument(
        '-d',
        '--makefile_directory',
        type=Path,
        metavar='DIR',
        default=default_makefile_directory,
        help=f'the directory containing the makefile invoked when building (default={default_makefile_directory})'
    )
    parser_run.add_argument(
        '-j',
        '--jobs',
        type=int,
        default=0,
        help=f'number of make jobs to start. 0 means use all available CPU '
            f'cores ({available_cpu_cores} on this system). Default=0'
    )
    parser_run.add_argument(
        '-l',
        '--load-average',
        type=int,
        default=0,
        help=f"Don't start multiple jobs unless load is below N. Same default "
            "values as --jobs"
    )
    parser_run.add_argument(
        '-m',
        '--macro-prefix',
        type=str,
        default='TS_ITCOIN_',
        help=f'The textual prefix that was added inside threadsafety.h. '
            f'Default=TS_ITCOIN_'
    )
    parser_run.add_argument(
        '-p',
        '--path',
        type=Path,
        default=default_path_prefix,
        help=f'The path where source code is looked for. If relative, it is '
            f'evaluated against the script directory ({script_dir}). '
            f'Default=../src, which resolves to {default_path_prefix}'
    )

    parser.epilog = textwrap.dedent(
        f"""\
        COMMANDS:
            {re.sub(r'^usage: ', '', parser_reconfigure.format_usage().rstrip())}
            {re.sub(r'^usage: ', '', parser_run.format_usage().rstrip())}

        To see the detailed help for each command, invoke:
            rename_threadsafety.py <reconfigure|run> --help
        """
    )

    args = parser.parse_args()
    if args.subcommand is None:
        parser.print_help()
        exit(1)

    if args.subcommand == 'reconfigure':
        if args.configure_script.is_absolute() == False:
            args.configure_script = (script_dir / args.configure_script)
        args.configure_script.resolve()
        if args.configure_script.exists() == False:
            logger.error(f'{args.configure_script} does not exist')
            exit(1)
    elif args.subcommand == 'run':
        if args.jobs == 0:
            args.jobs = available_cpu_cores
        if args.load_average == 0:
            args.load_average = available_cpu_cores
        if args.path.is_absolute() == False:
            args.path = (script_dir / args.path)
        args.path.resolve()
        if args.path.exists() == False:
            logger.error(f'{args.path} does not exist')
            exit(1)
        if args.path.is_dir() == False:
            logger.error(f'The given path ({args.path}) does not point to a directory')
            exit(1)
    else:
        logger.error(f'Invalid subcommand {args.subcommand}')
        exit(1)
    return args

def do_single_replacement_cycle(args: argparse.Namespace) -> Tuple[int, Set[Path]]:
    threadsafety_h_path: Path = args.path / "threadsafety.h"
    threadsafety_h_contents=threadsafety_h_path.read_text()

    linecache.clearcache()
    threadsafety_symbols=extract_threadsafety_symbols(threadsafety_h_contents, args.macro_prefix)
    logger.debug(f"These symbols are going to be prefixed with '{args.macro_prefix}': {threadsafety_symbols}. They were identified looking at the contents of {threadsafety_h_path}")
    if len(threadsafety_symbols) == 0:
        logger.warning(f"No symbols with the '{args.macro_prefix}' prefix were found. Did you remember to modify {threadsafety_h_path}?")
        exit(1)

    returncode, stderr = try_build_project(args.makefile_directory, args.jobs, args.load_average)
    (string_messages, structured_messages) = parse_gcc_output(stderr)

    if returncode == 0:
        logger.info("The build succeeded. Nothing to do.")
        return (0, set())

    if len(structured_messages) == 0:
        # If the build failed, but no json messages were slurped by
        # parse_gcc_output(), it is possible that the user forgot to enable
        # the json formatting of gcc output.
        #
        # In this case, the information we are interested in will be part of the
        # unstructured stderr. Let's print it so that the user can see his
        # overlook and fix it.
        for msg in string_messages:
            logger.info(f'>>> {msg}')
        logger.error(textwrap.dedent(f"""
            The build command failed with error code {returncode}, but no json messages were produced.
            Did you run:
                rename_threadsafety.py reconfigure
            Before attempting the first run of this tool?

            This will call the configuration script adding "-fdiagnostics-format=json" to the CXXFLAGS.

            For ease of diagnose, the captured stderr was printed above this message (prepended by ">>> ").
            """
        ))
        exit(returncode)

    gcc_errors = [ parse_raw_gcc_dict(gcc_error, args.path) for gcc_error in structured_messages ]

    # Filter all the elements of gcc_errors that are RawGccErrorRanged.
    # Apply ts_error_from_gcc_error() to all of them, and take only the ones for
    # which that function does not return None.
    #
    # This will be the list of errors from which we are going to build our
    # replacement plan.
    ts_errors = [ ts_error for ts_error in (
            ts_error_from_gcc_error(gcc_error, threadsafety_symbols)
                for gcc_error in gcc_errors if isinstance(gcc_error, RawGccErrorRanged)
        ) if ts_error
    ]

    if ts_errors == []:
        logger.debug("No renames found looking at the error message. Let's try to look at the line contents  and/or the source code")
        ts_errors = [ ts_error for ts_error in itertools.chain.from_iterable((
            (
                ts_error_from_gcc_error_raw_line(gcc_error, threadsafety_symbols),
                ts_error_from_gcc_error_raw_line_and_message(gcc_error, threadsafety_symbols),
                ts_error_from_gcc_error_not_a_type_with_fixit(gcc_error, threadsafety_symbols, args.macro_prefix)
            ) for gcc_error in gcc_errors),
        ) if ts_error]
        logger.debug(f"Found {len(ts_errors)} possible renames looking at the line contents")

    renaming_plan = build_renaming_plan(ts_errors, args.path)

    if (len(renaming_plan) == 0):
        for msg in string_messages:
            logger.info(f'>>> {msg}')
        for gcc_error in gcc_errors:
            logger.info(json.dumps(dataclasses.asdict(gcc_error), cls=PathEncoder))
        logger.error("The build failed and no possible renaming was identified. This is probably a bug or missing features in this program. The gcc errors where printed before this message")
        exit(returncode)

    logger.info(f"Identified a renaming plan. We are going to rewrite {len(renaming_plan)} files.")
    logger.info(f"This is the detailed renaming plan:")
    for fname, line_renspec in renaming_plan.items():
        for line, rensspec in line_renspec.items():
            logger.debug(f'{fname.relative_to(args.path)}:{line} {rensspec}')

    # TODO: use this instead:
    #
    # https://docs.python.org/3.7/library/fileinput.html#fileinput.FileInput
    # https://stackoverflow.com/questions/17140886/how-to-search-and-replace-text-in-a-file/20593644#20593644

    count_replacements = 0
    for fpath, lines_plan in renaming_plan.items():
        f_contents = fpath.read_text()
        new_contents = prefix_symbols_in_file(f_contents, lines_plan, args.macro_prefix)
        replacements_in_this_file = len(lines_plan.keys())
        logger.info(f"Rewriting {fpath.relative_to(args.path)} (done {replacements_in_this_file} replacements)")
        with open(fpath, "w") as out_f:
            out_f.write(new_contents)
        count_replacements += replacements_in_this_file

    logger.info(f"For this cycle, all the possible renaming where executed ({count_replacements} replacements in {len(renaming_plan.keys())} files)")
    return (count_replacements, set(renaming_plan.keys()))

def reconfigure_project(configure_script: Path) -> None:
    env = {
        'PATH': os.environ['PATH'],
        # https://gcc.gnu.org/onlinedocs/gcc-10.1.0/gcc/Diagnostic-Message-Formatting-Options.html
        "CXXFLAGS": "-fdiagnostics-format=json",
    }

    logger.info(f'Calling {configure_script} with env {env}')
    result = subprocess.run(
        [configure_script],
        env=env,
    )
    if result.returncode == 0:
        logger.info(f"SUCCESS. You can now execute {Path(__file__).name} run")
        exit(0)
    logger.error(f'The configuration was not successful (exit code {result.returncode})')
    exit(1)

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s %(name)s: %(message)s')
    logger = logging.getLogger(__name__)

    args = parse_cmdline()
    if args.subcommand == 'reconfigure':
        reconfigure_project(args.configure_script)
        exit(0)

    if args.subcommand != 'run':
        logger.error(f'Invalid command {args.subcommand}')
        exit(1)

    start = timer()
    total_compile_cycles = 0
    total_replacements = 0
    modified_files: Set[Path] = set()
    while True:
        total_compile_cycles += 1
        logger.info(f"Starting compile cycle {total_compile_cycles}")
        (cycle_replacements, cycle_modified_files) = do_single_replacement_cycle(args)
        if cycle_replacements == 0:
            break
        total_replacements += cycle_replacements
        modified_files |= cycle_modified_files
        logger.info(f"After {total_compile_cycles} compile cycles: a total of {total_replacements} replacements in {len(modified_files)} files")
    end = timer()
    elapsed_seconds = end -start
    logger.info(f"SUCCESS: a total of {total_replacements} replacements in {len(modified_files)} files over {total_compile_cycles} compile cycles were made. Elapsed time: {elapsed_seconds}s")
