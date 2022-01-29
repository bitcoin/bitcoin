[![Build Status](https://travis-ci.org/vpetrigo/caches.svg?branch=master)](https://travis-ci.org/vpetrigo/caches)
[![Build status](https://ci.appveyor.com/api/projects/status/5tcwwry337fbjgcb/branch/master?svg=true)](https://ci.appveyor.com/project/vpetrigo/caches/branch/master)

# C++ Cache implementation

This project implements a simple thread-safe cache with several page replacement policies:

  * Least Recently Used
  * First-In/First-Out
  * Least Frequently Used

More about cache algorithms and policy you could read on [Wikipedia](https://en.wikipedia.org/wiki/Cache_algorithms)

# Usage

Using this library is simple. It is necessary to include header with the cache implementation (`cache.hpp` file) 
and appropriate header with the cache policy if it is needed. If not then the non-special algorithm will be used (it removes
the last element which key is the last in the internal container).

Currently there is only three of them:

  * `fifo_cache_policy.hpp`
  * `lfu_cache_policy.hpp`
  * `lru_cache_policy.hpp`

Example for the LRU policy:

```cpp
#include <string>
#include "cache.hpp"
#include "lru_cache_policy.hpp"

// alias for easy class typing
template <typename Key, typename Value>
using lru_cache_t = typename caches::fixed_sized_cache<Key, Value, LRUCachePolicy<Key>>;

void foo(...) {
  constexpr std::size_t CACHE_SIZE = 256;
  lru_cache_t<std::string, int> cache(CACHE_SIZE);

  cache.Put("Hello", 1);
  cache.Put("world", 2);

  std::cout << cache.Get("Hello") << cache.Get("world") << std::endl;
  // "12"
}
```

# Requirements

The only requirement is a compatible C++11 compiler.

This project was tested in the environments listed below:

  * MinGW64 ([MSYS2 project](https://msys2.github.io/))
    * Clang 3.8.0
    * GCC 5.3.0
  * MSVC (VS 2015)
  * FreeBSD
    * Clang 3.4.1

If you have any issues with the library building, let me know please.

# Contributing

Please fork this repository and contribute back using [pull requests](https://github.com/vpetrigo/caches/pulls). 
Features can be requested using [issues](https://github.com/vpetrigo/caches/issues). All code, comments, and 
critiques are greatly appreciated.
