#include <fs.h>
#include <utilstrencodings.h>

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
#ifndef WIN32
    return ::fopen(p.string().c_str(), mode);
#else
    return ::_wfopen(p.wstring().c_str(), Utf8ToWide(mode).c_str());
#endif
}

FILE *freopen(const fs::path& p, const char *mode, FILE *stream)
{
#ifndef WIN32
    return ::freopen(p.string().c_str(), mode, stream);
#else
    return ::_wfreopen(p.wstring().c_str(), Utf8ToWide(mode).c_str(), stream);
#endif
}

} // fsbridge
