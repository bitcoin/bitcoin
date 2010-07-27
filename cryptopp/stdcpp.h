#ifndef CRYPTOPP_STDCPP_H
#define CRYPTOPP_STDCPP_H

#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <memory>
#include <string>
#include <exception>
#include <typeinfo>


#ifdef _MSC_VER
#include <string.h>	// CodeWarrior doesn't have memory.h
#include <algorithm>
#include <map>
#include <vector>

// re-disable this
#pragma warning(disable: 4231)
#endif

#if defined(_MSC_VER) && defined(_CRTAPI1)
#define CRYPTOPP_MSVCRT6
#endif

#endif
