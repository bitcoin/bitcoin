#include "config.h"
#include <iosfwd>
#include <string>

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4100 4189 4996)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wunused-variable"
#endif

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

#ifndef CRYPTOPP_UNUSED
# define CRYPTOPP_UNUSED(x) (void(x))
#endif

// Used for testing the compiler and linker in cryptest.sh
#if defined(CRYPTOPP_ADHOC_MAIN)

int main(int argc, char *argv[])
{
	CRYPTOPP_UNUSED(argc), CRYPTOPP_UNUSED(argv);
	return 0;
}

// Classic use of adhoc to setup calling convention
#else

extern int (*AdhocTest)(int argc, char *argv[]);

int MyAdhocTest(int argc, char *argv[])
{
	CRYPTOPP_UNUSED(argc), CRYPTOPP_UNUSED(argv);
	return 0;
}

static int s_i = (AdhocTest = &MyAdhocTest, 0);

#endif
