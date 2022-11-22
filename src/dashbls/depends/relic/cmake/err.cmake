include(CheckFunctionExists)
include(CheckIncludeFiles)

# Test for headers
check_include_files(execinfo.h HAVE_EXECINFO_H)

# Test for functions
check_function_exists(backtrace HAVE_BACKTRACE)
check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)
