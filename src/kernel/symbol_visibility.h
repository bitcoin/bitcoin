#ifndef BITCOIN_KERNEL_SYMBOL_VISIBILITY_H
#define BITCOIN_KERNEL_SYMBOL_VISIBILITY_H

#if defined(bitcoinkernel_EXPORTS)
  #if defined(_WIN32)
    #define BITCOINKERNEL_EXPORT __declspec(dllexport)
  #else
    #define BITCOINKERNEL_EXPORT __attribute__ ((visibility ("default")))
  #endif
#elif defined(_WIN32) && !defined(BITCOIN_BUILD_INTERNAL) && !defined(BITCOINKERNEL_STATIC)
  #define BITCOINKERNEL_EXPORT __declspec(dllimport)
#endif

#ifndef BITCOINKERNEL_EXPORT
  #define BITCOINKERNEL_EXPORT
#endif

#endif // BITCOIN_KERNEL_SYMBOL_VISIBILITY_H
