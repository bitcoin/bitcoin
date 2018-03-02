#include <fs.h>

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
    return ::fopen(p.string().c_str(), mode);
}

FILE *freopen(const fs::path& p, const char *mode, FILE *stream)
{
    return ::freopen(p.string().c_str(), mode, stream);
}

// Create and return temporary subdirectory for test outputs
fs::path unit_test_directory(){
    std::string sub_dir = "bitcoin_unit_tests";
    fs::path dir(fs::temp_directory_path() / sub_dir);

    if(fs::create_directory(dir) || fs::exists(dir))
    {
        return dir;
    }
    else{
        throw std::runtime_error("Error creating the directory\n");
    }
}

} // fsbridge
