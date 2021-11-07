#include <random.h>
#include <fs.h>
#include <util/strencodings.h>

fs::path GetUniquePath(const fs::path& base)
{
    FastRandomContext rnd;
    fs::path tmpFile = base / HexStr(rnd.randbytes(8));
    return tmpFile;
}