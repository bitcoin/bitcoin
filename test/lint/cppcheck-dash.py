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


def available_cpu_count():
    """Number of CPUs this process may actually run on.

    multiprocessing.cpu_count() reports every CPU the kernel knows about, which
    is the host's core count even when we are confined to a subset of them (a
    container or a cpuset-limited LXC guest, as used by the self-hosted CI
    runners). Oversubscribing cppcheck that way wastes time and memory, so
    prefer the scheduling affinity mask where the platform exposes one.
    """
    try:
        return len(os.sched_getaffinity(0))
    except AttributeError:
        return multiprocessing.cpu_count()

ALWAYS_ENABLED_WARNINGS = (
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

# Lines that indicate cppcheck itself failed to analyze a translation unit.
# These must always fail the lint (regardless of which file they point at),
# otherwise analysis silently ends up vacuous.
FATAL_ERRORS = (
    "preprocessorErrorDirective",
    "cppcheckError",
    "internalError",
    "Internal error",
    "syntaxError",
)

SUPPRESSED_WARNINGS = (
    "src/stacktraces.cpp:.*: .*: Parameter 'info' can be declared as pointer to const",
    "Return value 'state.(Invalid|Error).*' is always false.*knownConditionTrueFalse",
    "Local variable '_' shadows outer function.*shadowFunction",
    "Member variable 'ActiveDKG::.*' has no initializer.*uninitMemberVarNoCtor",
    "Member variable 'UtilParameters::.*' has no initializer.*uninitMemberVarNoCtor",

    "unusedFunction",
    "unknownMacro",
    "unusedStructMember",

    # Checks with pre-existing violations in the tree; suppressed wholesale so
    # the linter can be enforced. TODO: burn these down and re-enable them
    # one at a time. Note that any message matching ALWAYS_ENABLED_WARNINGS is
    # still reported even if its check id is listed here.
    "duplInheritedMember",
    "useStlAlgorithm",
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

    always_enabled_regexp = '|'.join(ALWAYS_ENABLED_WARNINGS)
    suppressed_regexp = '|'.join(SUPPRESSED_WARNINGS)
    fatal_regexp = '|'.join(FATAL_ERRORS)
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
        '-j', str(available_cpu_count()),
        '--language=c++',
        '--std=c++20',
        '--template=gcc',
        '--check-level=exhaustive',
        '-D__cplusplus',
        # Pretend to be GCC so that headers which require a known compiler
        # (e.g. src/attributes.h) don't hit an #error directive, which would
        # abort analysis of every translation unit that includes them.
        '-D__GNUC__',
        '-DENABLE_WALLET',
        '-DCLIENT_VERSION_BUILD',
        '-DCLIENT_VERSION_IS_RELEASE',
        '-DCLIENT_VERSION_MAJOR',
        '-DCLIENT_VERSION_MINOR',
        '-DCOPYRIGHT_YEAR',
        '-DDEBUG',
        '-DUSE_EPOLL',
        '-DCHAR_BIT=8',
        # Function-like macros that cppcheck cannot evaluate on its own; leaving
        # them undefined aborts analysis of Qt translation units with a fatal
        # syntaxError ("failed to evaluate #if condition").
        '-DQT_VERSION_CHECK(major,minor,patch)=((major<<16)|(minor<<8)|(patch))',
        '-DQT_CONFIG(x)=0',
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
        if re.search(fatal_regexp, line):
            warnings.append(line)
            continue
        # 'note:' and source-context lines only make sense next to their parent
        # warning; on their own (e.g. when the parent is suppressed) they are
        # noise, and they don't carry the check id the suppressions match on.
        # cppcheck's gcc template currently renders every non-error severity as
        # 'warning:', but match the raw severities too in case that changes.
        if not re.search(r' (?:error|warning|style|performance|portability): ', line):
            continue
        if not re.search(files_regexp, line):
            continue
        if re.search(always_enabled_regexp, line) or not re.search(suppressed_regexp, line):
            warnings.append(line)

    # Without --error-exitcode, diagnostics never make cppcheck return nonzero;
    # any nonzero status (bad arguments, unloadable config, OOM kill, crash)
    # means analysis did not complete and must not pass.
    rc = dependencies_output.returncode
    if rc != 0:
        print(f"cppcheck exited with code {rc}")
        if dependencies_output.stdout and not warnings:
            # Show a short tail to aid CI debugging without flooding logs.
            tail = dependencies_output.stdout.splitlines()[-50:]
            print('\n'.join(tail))
        exit_code = 1

    if warnings:
        print('\n'.join(warnings))
        print()
        print("Advice not applicable in this specific case? Add an exception by updating")
        print(f"SUPPRESSED_WARNINGS in {__file__}")
        # Uncomment to enforce the linter / comment to run locally
        exit_code = 1

    sys.exit(exit_code)

if __name__ == "__main__":
    main()
