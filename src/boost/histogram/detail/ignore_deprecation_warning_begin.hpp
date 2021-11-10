#if (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#elif defined(_MSC_VER)
#pragma warning(push)           // preserve warning settings
#pragma warning(disable : 4996) // disable depricated localtime/gmtime warning on vc8
#endif
