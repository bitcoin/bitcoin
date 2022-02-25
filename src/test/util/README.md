# Test library

This contains files for the test library, which is used by the test binaries (unit tests, benchmarks, fuzzers, gui
tests).

Generally, the files in this folder should be well-separated modules. New code should be added to existing modules or
(when in doubt) a new module should be created.

The utilities in here are compiled into a library, which does not hold any state. However, the main file `setup_common`
defines the common test setup for all test binaries. The test binaries will handle the global state when they
instantiate the `BasicTestingSetup` (or one of its derived classes).
