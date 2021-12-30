#!/usr/bin/env python3

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

def module_name(path):
    if path in MAPPING:
        path = MAPPING[path]
    if any(path.startswith(dirpath) for dirpath in HEADER_MODULE_PATHS):
        return path
    if path.endswith(".h"):
        return path[:-2]
    if path.endswith(".c"):
        return path[:-2]
    if path.endswith(".cpp"):
        return path[:-4]
    return None

if __name__=="__main__":
    files = dict()
    deps = dict()

    RE = re.compile("^#include <(.*)>")

    def handle_module(module):
        module = module_name(arg)
        if module is None:
            print("Ignoring file %s (does not constitute module)\n" % arg)
        else:
            files[arg] = module
            deps[module] = set()


    # Iterate over files, and create list of modules
    for arg in sys.argv[1:]:
        handle_module(arg)

    def build_list_direct(arg):
        module = files[arg]
        with open(arg, 'r', encoding="utf8") as f:
            for line in f:
                match = RE.match(line)
                if match:
                    include = match.group(1)
                    included_module = module_name(include)
                    if included_module is not None and included_module in deps and included_module != module:
                        deps[module].add(included_module)


    # Iterate again, and build list of direct dependencies for each module
    # TODO: implement support for multiple include directories
    for arg in sorted(files.keys()):
        build_list_direct(arg)
    # Loop to find the shortest (remaining) circular dependency

    def shortest_c_dep():
        have_cycle = False

        def handle_module(module, shortest_cycle):

            # Build the transitive closure of dependencies of module
            closure = dict()
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

            return shortest_cycle

        while True:

            shortest_cycles = None
            for module in sorted(deps.keys()):
                shortest_cycles = handle_module(module, shortest_cycles)

            if shortest_cycles is None:
                break
            # We have the shortest circular dependency; report it
            module = shortest_cycles[0]
            print("Circular dependency: %s" % (" -> ".join(shortest_cycles + [module])))
            # And then break the dependency to avoid repeating in other cycles
            deps[shortest_cycles[-1]] = deps[shortest_cycles[-1]] - set([module])
            have_cycle = True

        if have_cycle:
            return True

    sys.exit(1 if shortest_c_dep() else 0)
