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

#ifdef WIN32
static std::string openmodeToStr(std::ios_base::openmode mode)
{
    switch (mode & ~std::ios_base::ate) {
    case std::ios_base::out:
    case std::ios_base::out | std::ios_base::trunc:
        return "w";
    case std::ios_base::out | std::ios_base::app:
    case std::ios_base::app:
        return "a";
    case std::ios_base::in:
        return "r";
    case std::ios_base::in | std::ios_base::out:
        return "r+";
    case std::ios_base::in | std::ios_base::out | std::ios_base::trunc:
        return "w+";
    case std::ios_base::in | std::ios_base::out | std::ios_base::app:
    case std::ios_base::in | std::ios_base::app:
        return "a+";
    case std::ios_base::out | std::ios_base::binary:
    case std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
        return "wb";
    case std::ios_base::out | std::ios_base::app | std::ios_base::binary:
    case std::ios_base::app | std::ios_base::binary:
        return "ab";
    case std::ios_base::in | std::ios_base::binary:
        return "rb";
    case std::ios_base::in | std::ios_base::out | std::ios_base::binary:
        return "r+b";
    case std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
        return "w+b";
    case std::ios_base::in | std::ios_base::out | std::ios_base::app | std::ios_base::binary:
    case std::ios_base::in | std::ios_base::app | std::ios_base::binary:
        return "a+b";
    default:
        return nullptr;
    }
}

ifstream::ifstream(const fs::path& p, std::ios_base::openmode mode) : std::istream(nullptr)
{
    open(p, mode);
}

ifstream::~ifstream()
{
    close();
}

void ifstream::open(const fs::path& p, std::ios_base::openmode mode)
{
    close();
    std::string strmode = openmodeToStr(mode);
    file = fopen(p, strmode.c_str());
    if (file == nullptr) return;
    filebuf = std::move(__gnu_cxx::stdio_filebuf<char>(file, mode));
    rdbuf(&filebuf);
    if (mode & std::ios_base::ate) {
        seekg(0, std::ios_base::end);
    }
}

void ifstream::close()
{
    if (file != nullptr) {
        filebuf.close();
        fclose(file);
    }
    file = nullptr;
}

bool ifstream::is_open()
{
    return filebuf.is_open();
}

ofstream::ofstream(const fs::path& p, std::ios_base::openmode mode) : std::ostream(nullptr)
{
    open(p, mode);
}

ofstream::~ofstream()
{
    close();
}

void ofstream::open(const fs::path& p, std::ios_base::openmode mode)
{
    close();
    std::string strmode = openmodeToStr(mode);
    file = fopen(p, strmode.c_str());
    if (file == nullptr) return;
    filebuf = std::move(__gnu_cxx::stdio_filebuf<char>(file, mode));
    rdbuf(&filebuf);
    if (mode & std::ios_base::ate) {
        seekp(0, std::ios_base::end);
    }
}

void ofstream::close()
{
    if (file != nullptr) {
        filebuf.close();
        fclose(file);
    }
    file = nullptr;
}

bool ofstream::is_open()
{
    return filebuf.is_open();
}
#endif

} // fsbridge
