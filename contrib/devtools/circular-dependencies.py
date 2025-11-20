#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import collections
import fnmatch
import sys
import re

MAPPING = {
    'core_read.cpp': 'core_io.cpp',
    'core_write.cpp': 'core_io.cpp',
}

# Directories with header-based modules, where the assumption that .cpp files
# define functions and variables declared in corresponding .h files is
# incorrect.
HEADER_MODULE_PATHS = [
    'interfaces/'
]

# Headers whose dependencies are ignored, but will trigger errors if included
# transitively (from other headers instead of source files). This can be useful
# for header-only modules whose dependencies should not trigger circular
# dependency errors between other modules. Specifically it is useful for
# *_settings.h files which are only meant to be included by .cpp files
# registering and retrieving settings, and which may include headers in the same
# modules as those .cpp files for setting types and default values. As long as
# .cpp files are following IWYU rules and directly including headers they use,
# this should not cause circular dependencies to go undetected.
NONTRANSITIVE_HEADERS = [
    '**_settings.h'
]

def module_name(path):
    if path in MAPPING:
        path = MAPPING[path]
    if any(path.startswith(dirpath) for dirpath in HEADER_MODULE_PATHS):
        return path
    if any(fnmatch.fnmatch(path, pat) for pat in NONTRANSITIVE_HEADERS):
        return False
    if path.endswith(".h"):
        return path[:-2]
    if path.endswith(".c"):
        return path[:-2]
    if path.endswith(".cpp"):
        return path[:-4]
    return None

files = dict()
deps: dict[str, set[str]] = dict()

RE = re.compile("^#include <(.*)>")

# Iterate over files, and create list of modules
for arg in sys.argv[1:]:
    module = module_name(arg)
    if module is None:
        print("Ignoring file %s (does not constitute module)\n" % arg)
    elif module is not False:
        files[arg] = module
        deps[module] = set()

# Iterate again, and build list of direct dependencies for each module
# TODO: implement support for multiple include directories
included_by = collections.defaultdict(set)
for arg in sorted(files.keys()):
    module = files[arg]
    with open(arg, 'r', encoding="utf8") as f:
        for line in f:
            match = RE.match(line)
            if match:
                include = match.group(1)
                included_module = module_name(include)
                if included_module and included_module in deps and included_module != module:
                    deps[module].add(included_module)
                included_by[include].add(arg)

# Trigger an error if any module listed as being nontransitive is ever included
# indirectly. The assumption that nontransitive modules do not trigger circular
# dependencies is only valid when they are included directly.
for arg in sys.argv[1:]:
    if module_name(arg) is not False:
        continue
    for path in included_by[arg]:
        if included_by[path]:
            raise Exception(f"Error: file {arg} which is listed in NONTRANSITIVE_HEADERS is included indirectly, through {path}, by {included_by[path]}. Files listed in NONTRANSITIVE_HEADERS should only be included by .cpp files, not .h files.")

# Loop to find the shortest (remaining) circular dependency
have_cycle: bool = False
while True:
    shortest_cycle = None
    for module in sorted(deps.keys()):
        # Build the transitive closure of dependencies of module
        closure: dict[str, list[str]] = dict()
        for dep in deps[module]:
            closure[dep] = []
        while True:
            old_size = len(closure)
            old_closure_keys = sorted(closure.keys())
            for src in old_closure_keys:
                for dep in deps[src]:
                    if dep not in closure:
                        closure[dep] = closure[src] + [src]
            if len(closure) == old_size:
                break
        # If module is in its own transitive closure, it's a circular dependency; check if it is the shortest
        if module in closure and (shortest_cycle is None or len(closure[module]) + 1 < len(shortest_cycle)):
            shortest_cycle = [module] + closure[module]
    if shortest_cycle is None:
        break
    # We have the shortest circular dependency; report it
    module = shortest_cycle[0]
    print("Circular dependency: %s" % (" -> ".join(shortest_cycle + [module])))
    # And then break the dependency to avoid repeating in other cycles
    deps[shortest_cycle[-1]] = deps[shortest_cycle[-1]] - set([module])
    have_cycle = True

sys.exit(1 if have_cycle else 0)
