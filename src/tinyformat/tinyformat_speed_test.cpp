#if defined(__linux__) && defined(__clang__)
// Workaround for bug in gcc 4.4 standard library headers when compling with
// clang in C++11 mode.
namespace std { class type_info; }
#endif

#include <boost/format.hpp>
#include <iomanip>
#include <stdio.h>
#include "tinyformat.h"

void speedTest(const std::string& which)
{
    // Following is required so that we're not limited by per-character
    // buffering.
    std::ios_base::sync_with_stdio(false);
    const long maxIter = 2000000L;
    if(which == "printf")
    {
        // libc version
        for(long i = 0; i < maxIter; ++i)
            printf("%0.10f:%04d:%+g:%s:%p:%c:%%\n",
                   1.234, 42, 3.13, "str", (void*)1000, (int)'X');
    }
    else if(which == "iostreams")
    {
        // Std iostreams version.  What a mess!!
        for(long i = 0; i < maxIter; ++i)
            std::cout
                << std::setprecision(10) << std::fixed << 1.234
                << std::resetiosflags(std::ios::floatfield) << ":"
                << std::setw(4) << std::setfill('0') << 42 << std::setfill(' ') << ":"
                << std::setiosflags(std::ios::showpos) << 3.13 << std::resetiosflags(std::ios::showpos) << ":"
                << "str" << ":"
                << (void*)1000 << ":"
                << 'X' << ":%\n";
    }
    else if(which == "tinyformat")
    {
        // tinyformat version.
        for(long i = 0; i < maxIter; ++i)
            tfm::printf("%0.10f:%04d:%+g:%s:%p:%c:%%\n",
                        1.234, 42, 3.13, "str", (void*)1000, (int)'X');
    }
    else if(which == "boost")
    {
        // boost::format version
        for(long i = 0; i < maxIter; ++i)
            std::cout << boost::format("%0.10f:%04d:%+g:%s:%p:%c:%%\n")
                % 1.234 % 42 % 3.13 % "str" % (void*)1000 % (int)'X';
    }
    else
    {
        assert(0 && "speed test for which version?");
    }
}


int main(int argc, char* argv[])
{
    if(argc >= 2)
        speedTest(argv[1]);
    return 0;
}
