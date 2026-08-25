#!/usr/bin/env python3
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys
import re
import os

# Directories with header-based modules, where the assumption that .cpp files
# define functions and variables declared in corresponding .h files is
# incorrect.
HEADER_MODULE_PATHS = [
    'interfaces/'
]


def normalize_path(path):
    return os.path.normpath(path).replace('\\', '/').lstrip('./')


def module_name(path):
    normalized = normalize_path(path)
    if any(normalized == dirpath.rstrip('/') or normalized.startswith(dirpath.rstrip('/') + '/') for dirpath in HEADER_MODULE_PATHS):
        return normalized
    if normalized.endswith(".h"):
        return normalized[:-2]
    if normalized.endswith(".c"):
        return normalized[:-2]
    if normalized.endswith(".cpp"):
        return normalized[:-4]
    return None


def resolve_include_path(include, source_dir, include_dirs):
    candidates = [
        normalize_path(include),
        normalize_path(os.path.join(source_dir, include)),
    ]
    for include_dir in include_dirs:
        candidates.append(normalize_path(os.path.join(include_dir, include)))

    for candidate in candidates:
        module = module_name(candidate)
        if module is not None:
            return module
    return None


files = dict()
deps: dict[str, set[str]] = dict()

RE = re.compile("^#include <(.*)>")

include_dirs = ['.']
argv = sys.argv[1:]
while argv and argv[0].startswith('-I'):
    include_dir = argv[0][2:]
    if not include_dir:
        include_dir = argv[1]
        argv = argv[1:]
    include_dirs.append(include_dir)
    argv = argv[1:]

# Iterate over files, and create list of modules
for arg in argv:
    module = module_name(arg)
    if module is None:
        print("Ignoring file %s (does not constitute module)\n" % arg)
    else:
        files[arg] = module
        deps[module] = set()

# Iterate again, and build list of direct dependencies for each module
for arg in sorted(files.keys()):
    module = files[arg]
    with open(arg, 'r') as f:
        for line in f:
            match = RE.match(line)
            if match:
                include = match.group(1)
                included_module = resolve_include_path(include, os.path.dirname(arg), include_dirs)
                if included_module is not None and included_module in deps and included_module != module:
                    deps[module].add(included_module)

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
