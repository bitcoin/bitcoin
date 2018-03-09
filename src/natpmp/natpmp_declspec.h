#ifndef DECLSPEC_H_INCLUDED
#define DECLSPEC_H_INCLUDED

#if defined(WIN32) && !defined(NATPMP_STATICLIB)
    /* for windows dll */
    #ifdef NATPMP_EXPORTS
        #define NATPMP_LIBSPEC __declspec(dllexport)
    #else
        #define NATPMP_LIBSPEC __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        /* fix dynlib for OS X 10.9.2 and Apple LLVM version 5.0 */
        #define NATPMP_LIBSPEC __attribute__ ((visibility ("default")))
    #else
        #define NATPMP_LIBSPEC
    #endif
#endif

#endif

