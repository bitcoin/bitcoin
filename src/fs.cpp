#include <fs.h>
#include <utilstrencodings.h>

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
    return ::fopen(p.string().c_str(), mode);
}

FILE *freopen(const fs::path& p, const char *mode, FILE *stream)
{
    return ::freopen(p.string().c_str(), mode, stream);
}

Path::Path() : fs::path(){}

Path::Path(const fs::path& p) : fs::path(p) {}

std::string Path::u8string() const
{
    return NativeToUtf8(boost::filesystem::path::string());
}

Path U8Path(const std::string& source)
{
    return Utf8ToNative(source);
}
} // fsbridge
