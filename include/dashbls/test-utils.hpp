// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <string>
#include "bls.hpp"

using std::string;
using std::vector;
using std::cout;
using std::endl;

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif
#define STR(x) #x
#define ASSERT(x) if (!(x)) { printf("BLS assertion failed: (%s), function %s, file %s, line %d.\n", STR(x), __PRETTY_FUNCTION__, __FILE__, __LINE__); abort(); }

std::chrono::time_point<std::chrono::steady_clock> startStopwatch() {
    return std::chrono::steady_clock::now();
}

void endStopwatch(string testName,
                  std::chrono::time_point<std::chrono::steady_clock> start,
                  int numIters) {
    auto end = std::chrono::steady_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    cout << endl << testName << endl;
    cout << "Total: " << numIters << " runs in " << now_ms.count()
         << " ms" << endl;
    cout << "Avg: " << now_ms.count() / static_cast<double>(numIters)
         << " ms" << endl;
}

std::vector<uint8_t> getRandomSeed() {
    uint8_t buf[32];
    bn_t r;
    bn_new(r);
    bn_rand(r, RLC_POS, 256);
    bn_write_bin(buf, 32, r);
    std::vector<uint8_t> ret(buf, buf + 32);
    return ret;
}
