/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
/*
 * Kill -
 * Simulate Unix kill on Windows/NT and Windows/9X.
 * This good enough to support the Berkeley DB test suite,
 * but may be missing some favorite features.
 *
 * Would have used MKS kill, but it didn't seem to work well
 * on Win/9X.  Cygnus kill works within the Gnu/Cygnus environment
 * (where processes are given small pids, with presumably a translation
 * table between small pids and actual process handles), but our test
 * environment, via Tcl, does not use the Cygnus environment.
 *
 * Compile this and install it as c:/tools/kill.exe (or as indicated
 * by build_windows/include.tcl ).
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/*
 * Like atol, with specified base.  Would use stdlib, but
 * strtol("0xFFFF1234", NULL, 16) returns 0x7FFFFFFF and
 * strtol("4294712487", NULL, 16) returns 0x7FFFFFFF w/ VC++
 */
long
myatol(char *s, int base)
{
    long result = 0;
    char ch;
    int sign = 1;  /* + */
    if (base == 0)
        base = 10;
    if (base != 10 && base != 16)
        return LONG_MAX;
    while ((ch = *s++) != '\0') {
        if (ch == '-') {
            sign = -sign;
        }
        else if (ch >= '0' && ch <= '9') {
            result = result * base + (ch - '0');
        }
        else if (ch == 'x' || ch == 'X') {
            /* Allow leading 0x..., and switch to base 16 */
            base = 16;
        }
        else if (base == 16 && ch >= 'a' && ch <= 'f') {
            result = result * base + (ch - 'a' + 10);
        }
        else if (base == 16 && ch >= 'A' && ch <= 'F') {
            result = result * base + (ch - 'A' + 10);
        }
        else {
            if (sign > 1)
                return LONG_MAX;
            else
                return LONG_MIN;
        }
    }
    return sign * result;
}

void
usage_exit()
{
    fprintf(stderr, "Usage: kill [ -sig ] pid\n");
    fprintf(stderr, "       for win32, sig must be or 0, 15 (TERM)\n");
    exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
    HANDLE hProcess ;
    DWORD accessflag;
    long pid;
    int sig = 15;

    if (argc > 2) {
        if (argv[1][0] != '-')
            usage_exit();

        if (strcmp(argv[1], "-TERM") == 0)
            sig = 15;
        else {
            /* currently sig is more or less ignored,
             * we only care if it is zero or not
             */
            sig = atoi(&argv[1][1]);
            if (sig < 0)
                usage_exit();
        }
        argc--;
        argv++;
    }
    if (argc < 2)
        usage_exit();

    pid = myatol(argv[1], 10);
    /*printf("pid = %ld (0x%lx) (command line %s)\n", pid, pid, argv[1]);*/
    if (pid == LONG_MAX || pid == LONG_MIN)
        usage_exit();

    if (sig == 0)
        accessflag = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    else
        accessflag = STANDARD_RIGHTS_REQUIRED | PROCESS_TERMINATE;
    hProcess = OpenProcess(accessflag, FALSE, pid);
    if (hProcess == NULL) {
        fprintf(stderr, "dbkill: %s: no such process\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    if (sig == 0)
        exit(EXIT_SUCCESS);
    if (!TerminateProcess(hProcess, 99)) {
        DWORD err = GetLastError();
        fprintf(stderr,
            "dbkill: cannot kill process: error %d (0x%lx)\n", err, err);
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
