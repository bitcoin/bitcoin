#ifndef _UINT256_T_CONFIG_
  #define _UINT256_T_CONFIG_
  #if defined(_MSC_VER)
    #if defined(_DLL)
      #define _UINT256_T_EXPORT __declspec(dllexport)
      #define _UINT256_T_IMPORT __declspec(dllimport)
    #else
      #define _UINT256_T_EXPORT
      #define _UINT256_T_IMPORT
    #endif
  #else
    // All modules on Unix are compiled with -fvisibility=hidden
    // All API symbols get visibility default
    // whether or not we're static linking or dynamic linking (with -fPIC)
    #define _UINT256_T_EXPORT __attribute__((visibility("default"))) 
    #define _UINT256_T_IMPORT __attribute__((visibility("default"))) 
  #endif
#endif

