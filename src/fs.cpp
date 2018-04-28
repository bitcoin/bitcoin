#include <fs.h>
#include <utilstrencodings.cpp>

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
    return ::fopen(Utf8ToLocal(p.string()).c_str(), mode);
}

FILE *freopen(const fs::path& p, const char *mode, FILE *stream)
{
    return ::freopen(Utf8ToLocal(p.string()).c_str(), mode, stream);
}

} // fsbridge
