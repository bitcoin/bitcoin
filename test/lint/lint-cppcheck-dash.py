#!/usr/bin/env python3
#
# Copyright (c) 2019 The Bitcoin Core developers
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Run cppcheck for dash specific files

import multiprocessing
import os
import re
import subprocess
import sys

os.environ['LC_ALL'] = 'C'

ENABLED_CHECKS = (
    "Class '.*' has a constructor with 1 argument that is not explicit.",
    "Struct '.*' has a constructor with 1 argument that is not explicit.",
    "Function parameter '.*' should be passed by const reference.",
    "Comparison of modulo result is predetermined",
    "Local variable '.*' shadows outer argument",
    "Redundant initialization for '.*'. The initialized value is overwritten before it is read.",
    "Dereferencing '.*' after it is deallocated / released",
    "The scope of the variable '.*' can be reduced.",
    "Parameter '.*' can be declared with const",
    "Variable '.*' can be declared with const",
    "Variable '.*' is assigned a value that is never used.",
    "Unused variable",
    "The function '.*' overrides a function in a base class but is not marked with a 'override' specifier.",
    # Enable to catch all warnings
    # ".*",
)

IGNORED_WARNINGS = (
    "src/bls/bls.h:.* Struct 'CBLSIdImplicit' has a constructor with 1 argument that is not explicit.",
    "src/rpc/masternode.cpp:.*:21: warning: Consider using std::copy algorithm instead of a raw loop.",  # UniValue doesn't support std::copy
    "src/cachemultimap.h:.*: warning: Variable 'mapIt' can be declared as reference to const",
    "src/evo/simplifiedmns.cpp:.*:20: warning: Consider using std::copy algorithm instead of a raw loop.",
    "src/llmq/commitment.cpp.* warning: Consider using std::all_of or std::none_of algorithm instead of a raw loop. [useStlAlgorithm]",
    "src/rpc/.*cpp:.*: note: Function pointer used here.",
    "src/masternode/sync.cpp:.*: warning: Variable 'pnode' can be declared as pointer to const [constVariableReference]",
    "src/wallet/bip39.cpp.*: warning: The scope of the variable 'ssCurrentWord' can be reduced. [variableScope]",
    "src/.*:.*: warning: Local variable '_' shadows outer function [shadowFunction]",

    "src/stacktraces.cpp:.*: .*: Parameter 'info' can be declared as pointer to const",
    "src/stacktraces.cpp:.*: note: You might need to cast the function pointer here",

    "[note|warning]: Return value 'state.Invalid(.*)' is always false",
    "note: Calling function 'Invalid' returns 0",
    "note: Shadow variable",

    # General catchall, for some reason any value named 'hash' is viewed as never used.
    "Variable 'hash' is assigned a value that is never used.",

    # The following can be useful to ignore when the catch all is used
    # "Consider performing initialization in initialization list.",
    "Consider using std::transform algorithm instead of a raw loop.",
    "Consider using std::accumulate algorithm instead of a raw loop.",
    "Consider using std::any_of algorithm instead of a raw loop.",
    "Consider using std::copy_if algorithm instead of a raw loop.",
    # "Consider using std::count_if algorithm instead of a raw loop.",
    # "Consider using std::find_if algorithm instead of a raw loop.",
    # "Member variable '.*' is not initialized in the constructor.",

    "unusedFunction",
    "unknownMacro",
    "unusedStructMember",
)

def main():
    warnings = []
    exit_code = 0

    try:
        subprocess.check_output(['cppcheck', '--version'])
    except FileNotFoundError:
        print("Skipping cppcheck linting since cppcheck is not installed.")
        sys.exit(0)

    with open('test/util/data/non-backported.txt', 'r', encoding='utf-8') as f:
        patterns = [line.strip() for line in f if line.strip()]

    files_output = subprocess.check_output(['git', 'ls-files', '--'] + patterns, universal_newlines=True, encoding="utf8")
    files = [f.strip() for f in files_output.splitlines() if f.strip()]

    enabled_regexp = '|'.join(ENABLED_CHECKS)
    ignored_regexp = '|'.join(IGNORED_WARNINGS)
    files_regexp = '|'.join(re.escape(f) for f in files)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    cache_dir = os.environ.get('CACHE_DIR')
    if cache_dir:
        cppcheck_dir = os.path.join(cache_dir, 'cppcheck')
    else:
        cppcheck_dir = os.path.join(script_dir, '.cppcheck')
    os.makedirs(cppcheck_dir, exist_ok=True)

    cppcheck_cmd = [
        'cppcheck',
        '--enable=all',
        '--inline-suppr',
        '--suppress=missingIncludeSystem',
        f'--cppcheck-build-dir={cppcheck_dir}',
        '-j', str(multiprocessing.cpu_count()),
        '--language=c++',
        '--std=c++20',
        '--template=gcc',
        '-D__cplusplus',
        '-DENABLE_WALLET',
        '-DCLIENT_VERSION_BUILD',
        '-DCLIENT_VERSION_IS_RELEASE',
        '-DCLIENT_VERSION_MAJOR',
        '-DCLIENT_VERSION_MINOR',
        '-DCOPYRIGHT_YEAR',
        '-DDEBUG',
        '-DUSE_EPOLL',
        '-DCHAR_BIT=8',
        '-I', 'src/',
        '-q',
    ] + files

    dependencies_output = subprocess.run(
        cppcheck_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    unique_sorted_lines = sorted(set(dependencies_output.stdout.splitlines()))
    for line in unique_sorted_lines:
        if re.search(enabled_regexp, line) and not re.search(ignored_regexp, line) and re.search(files_regexp, line):
            warnings.append(line)

    if warnings:
        print('\n'.join(warnings))
        print()
        print("Advice not applicable in this specific case? Add an exception by updating")
        print(f"IGNORED_WARNINGS in {__file__}")
        # Uncomment to enforce the linter / comment to run locally
        exit_code = 1

    sys.exit(exit_code)

if __name__ == "__main__":
    main()
