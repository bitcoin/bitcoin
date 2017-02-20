# Features we don't test for, but want the #defines to exist for
# other ports.
AH_TEMPLATE(DB_WIN32,
    [We use DB_WIN32 much as one would use _WIN32 -- to specify that
    we're using an operating system environment that supports Win32
    calls and semantics.  We don't use _WIN32 because Cygwin/GCC also
    defines _WIN32, even though Cygwin/GCC closely emulates the Unix
    environment.])

AH_TEMPLATE(HAVE_BREW, [Define to 1 if building on BREW.])
AH_TEMPLATE(HAVE_BREW_SDK2, [Define to 1 if building on BREW (SDK2).])
AH_TEMPLATE(HAVE_S60, [Define to 1 if building on S60.])
AH_TEMPLATE(HAVE_VXWORKS, [Define to 1 if building on VxWorks.])

AH_TEMPLATE(HAVE_FILESYSTEM_NOTZERO,
    [Define to 1 if allocated filesystem blocks are not zeroed.])

AH_TEMPLATE(HAVE_UNLINK_WITH_OPEN_FAILURE,
    [Define to 1 if unlink of file with open file descriptors will fail.])

AH_TEMPLATE(HAVE_SYSTEM_INCLUDE_FILES,
    [Define to 1 if port includes files in the Berkeley DB source code.])
