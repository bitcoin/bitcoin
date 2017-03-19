#pragma once

#ifdef _MSC_VER
    // warning C4503: '__LINE__Var': decorated name length exceeded, name was truncated
    #pragma warning(disable: 4503)
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

#include "uint256.h"
#include "streams.h"
#include "serialize.h"
