// This file is a modified version of Python 2.2/2.2.1 Python.h. As
// such it is:
//
//    Copyright (c) 2001, 2002 Python Software Foundation; All Rights
//    Reserved
//
// boostinspect:nolicense (don't complain about the lack of a Boost license)
//
// Changes from the original:
//  1. #includes <unistd.h> for Python 2.2.1
//  2. Provides missing extern "C" wrapper for  "iterobject.h" and  "descrobject.h".
//

// Changes marked with "Boost.Python modification"
#ifndef Py_PYTHON_H
#define Py_PYTHON_H
/* Since this is a "meta-include" file, no #ifdef __cplusplus / extern "C" { */


/* Enable compiler features; switching on C lib defines doesn't work
   here, because the symbols haven't necessarily been defined yet. */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE    1
#endif

/* Forcing SUSv2 compatibility still produces problems on some
   platforms, True64 and SGI IRIX begin two of them, so for now the
   define is switched off. */
#if 0
#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE  500
#endif
#endif

/* Include nearly all Python header files */

#include "patchlevel.h"
#include "pyconfig.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

/* pyconfig.h may or may not define DL_IMPORT */
#ifndef DL_IMPORT       /* declarations for DLL import/export */
#define DL_IMPORT(RTYPE) RTYPE
#endif
#ifndef DL_EXPORT       /* declarations for DLL import/export */
#define DL_EXPORT(RTYPE) RTYPE
#endif

#if defined(__sgi) && defined(WITH_THREAD) && !defined(_SGI_MP_SOURCE)
#define _SGI_MP_SOURCE
#endif

#include <stdio.h>
#ifndef NULL
#   error "Python.h requires that stdio.h define NULL."
#endif

#include <string.h>
#include <errno.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if PY_MICRO_VERSION == 1 // Boost.Python modification: emulate Python 2.2
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif // Boost.Python modification: emulate Python 2.2

/* CAUTION:  Build setups should ensure that NDEBUG is defined on the
 * compiler command line when building Python in release mode; else
 * assert() calls won't be removed.
 */
#include <assert.h>

#include "pyport.h"

#include "pymem.h"

#include "object.h"
#include "objimpl.h"

#include "pydebug.h"

#include "unicodeobject.h"
#include "intobject.h"
#include "longobject.h"
#include "floatobject.h"
#ifndef WITHOUT_COMPLEX
#include "complexobject.h"
#endif
#include "rangeobject.h"
#include "stringobject.h"
#include "bufferobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "funcobject.h"
#include "classobject.h"
#include "fileobject.h"
#include "cobject.h"
#include "traceback.h"
#include "sliceobject.h"
#include "cellobject.h"
extern "C" { // Boost.Python modification: provide missing extern "C"
#include "iterobject.h"
#include "descrobject.h"
} // Boost.Python modification: provide missing extern "C"
#include "weakrefobject.h"

#include "codecs.h"
#include "pyerrors.h"

#include "pystate.h"

#include "modsupport.h"
#include "pythonrun.h"
#include "ceval.h"
#include "sysmodule.h"
#include "intrcheck.h"
#include "import.h"

#include "abstract.h"

#define PyArg_GetInt(v, a)      PyArg_Parse((v), "i", (a))
#define PyArg_NoArgs(v)         PyArg_Parse(v, "")

/* Convert a possibly signed character to a nonnegative int */
/* XXX This assumes characters are 8 bits wide */
#ifdef __CHAR_UNSIGNED__
#define Py_CHARMASK(c)          (c)
#else
#define Py_CHARMASK(c)          ((c) & 0xff)
#endif

#include "pyfpe.h"

/* These definitions must match corresponding definitions in graminit.h.
   There's code in compile.c that checks that they are the same. */
#define Py_single_input 256
#define Py_file_input 257
#define Py_eval_input 258

#ifdef HAVE_PTH
/* GNU pth user-space thread support */
#include <pth.h>
#endif
#endif /* !Py_PYTHON_H */
