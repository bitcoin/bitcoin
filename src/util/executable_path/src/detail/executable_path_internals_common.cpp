//
// Copyright (C) 2011-2019 Ben Key
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <locale>
#include <iterator>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/locale.hpp>
#include <boost/tokenizer.hpp>

#if (BOOST_VERSION >= 106100 && BOOST_VERSION < 106400)
#  include <boost/dll/runtime_symbol_info.hpp>
#endif

#if (BOOST_VERSION >= 106400)
#  include <boost/process.hpp>
#endif

#if (BOOST_OS_CYGWIN || BOOST_OS_WINDOWS)
#  include <windows.h>
#endif

#include <util/executable_path/include/boost/detail/executable_path_internals.hpp>


namespace boost {
namespace detail {
// NOLINTNEXTLINE(clang-diagnostic-unused-const-variable)
const size_t buffer_size = 8192;
const size_t default_converter_max_length = 6;

std::string os_pathsep()
{
#if (BOOST_OS_WINDOWS)
    return ";";
#else
    return ":";
#endif
}

std::wstring wos_pathsep()
{
#if (BOOST_OS_WINDOWS)
    return L";";
#else
    return L":";
#endif
}

std::string os_sep()
{
#if (BOOST_OS_WINDOWS)
    return "\\";
#else
    return "/";
#endif
}

std::wstring wos_sep()
{
#if (BOOST_OS_WINDOWS)
    return L"\\";
#else
    return L"/";
#endif
}

bool IsUTF8(const std::locale& loc)
{
    std::string locName = loc.name();
    return (!locName.empty() && std::string::npos != locName.find("UTF-8"));
}

std::string to_string(const std::wstring& s, const std::locale& loc)
{
    using char_vector = std::vector<char>;
    using converter_type = std::codecvt<wchar_t, char, std::mbstate_t>;
    using wchar_facet = std::ctype<wchar_t>;
    std::string return_value;
    if (s.empty())
    {
        return "";
    }
    if (IsUTF8(loc))
    {
        return_value = boost::locale::conv::utf_to_utf<char>(s);
        if (!return_value.empty())
        {
            return return_value;
        }
    }
    const wchar_t* from = s.c_str();
    size_t len = s.length();
    size_t converterMaxLength = default_converter_max_length;
    size_t bufferSize = ((len + default_converter_max_length) * converterMaxLength);
    if (std::has_facet<converter_type>(loc))
    {
        const auto& converter = std::use_facet<converter_type>(loc);
        if (!converter.always_noconv())
        {
            converterMaxLength = converter.max_length();
            if (converterMaxLength != default_converter_max_length)
            {
                bufferSize = ((len + default_converter_max_length) * converterMaxLength);
            }
            std::mbstate_t state;
            const wchar_t* from_next = nullptr;
            char_vector to(bufferSize, 0);
            char* toPtr = to.data();
            char* to_next = nullptr;
            const converter_type::result result = converter.out(
                state, from, from + len, from_next, toPtr, toPtr + bufferSize, to_next);
            if ((converter_type::ok == result || converter_type::noconv == result) && 0 != toPtr[0])
            {
                return_value.assign(toPtr, to_next);
            }
        }
    }
    if (return_value.empty() && std::has_facet<wchar_facet>(loc))
    {
        char_vector to(bufferSize, 0);
        auto toPtr = to.data();
        const auto& facet = std::use_facet<wchar_facet>(loc);
        if (facet.narrow(from, from + len, '?', toPtr) != nullptr)
        {
            return_value = toPtr;
        }
    }
    return return_value;
}

std::wstring to_wstring(const std::string& s, const std::locale& loc)
{
    using wchar_vector = std::vector<wchar_t>;
    using wchar_facet = std::ctype<wchar_t>;
    std::wstring return_value;
    if (s.empty())
    {
        return L"";
    }
    if (IsUTF8(loc))
    {
        return_value = boost::locale::conv::utf_to_utf<wchar_t>(s);
        if (!return_value.empty())
        {
            return return_value;
        }
    }
    if (std::has_facet<wchar_facet>(loc))
    {
        std::string::size_type bufferSize = s.size() + 2;
        wchar_vector to(bufferSize, 0);
        wchar_t* toPtr = to.data();
        const auto& facet = std::use_facet<wchar_facet>(loc);
        if (facet.widen(s.c_str(), s.c_str() + s.size(), toPtr) != nullptr)
        {
            return_value = toPtr;
        }
    }
    return return_value;
}

std::string GetEnv(const std::string& varName)
{
    if (varName.empty())
    {
        return "";
    }
#if (BOOST_OS_BSD || BOOST_OS_CYGWIN || BOOST_OS_LINUX || BOOST_OS_MACOS || BOOST_OS_SOLARIS)
    char* value = std::getenv(varName.c_str());
    if (value == nullptr)
    {
        return "";
    }
    return value;
#elif (BOOST_OS_WINDOWS)
    using char_vector = std::vector<char>;
    using size_type = std::vector<char>::size_type;
    char_vector value(buffer_size, 0);
    size_type size = value.size();
    bool haveValue = false;
    bool shouldContinue = true;
    do
    {
        DWORD result = GetEnvironmentVariableA(varName.c_str(), value.data(), size);
        if (result == 0)
        {
            shouldContinue = false;
        }
        else if (result < size)
        {
            haveValue = true;
            shouldContinue = false;
        }
        else
        {
            size *= 2;
            value.resize(size);
        }
    } while (shouldContinue);
    std::string ret;
    if (haveValue)
    {
        ret = value.data();
    }
    return ret;
#else
    return "";
#endif
}

std::wstring GetEnv(const std::wstring& varName)
{
    if (varName.empty())
    {
        return L"";
    }
#if (BOOST_OS_BSD || BOOST_OS_CYGWIN || BOOST_OS_LINUX || BOOST_OS_MACOS || BOOST_OS_SOLARIS)
    std::locale loc;
    std::string sVarName = to_string(varName, loc);
    char* value = std::getenv(sVarName.c_str());
    if (value == nullptr)
    {
        return L"";
    }
    std::wstring ret = to_wstring(value, loc);
    return ret;
#elif (BOOST_OS_WINDOWS)
    using char_vector = std::vector<wchar_t>;
    using size_type = std::vector<wchar_t>::size_type;
    char_vector value(buffer_size, 0);
    size_type size = value.size();
    bool haveValue = false;
    bool shouldContinue = true;
    do
    {
        DWORD result = GetEnvironmentVariableW(varName.c_str(), value.data(), size);
        if (result == 0)
        {
            shouldContinue = false;
        }
        else if (result < size)
        {
            haveValue = true;
            shouldContinue = false;
        }
        else
        {
            size *= 2;
            value.resize(size);
        }
    } while (shouldContinue);
    std::wstring ret;
    if (haveValue)
    {
        ret = value.data();
    }
    return ret;
#else
    return L"";
#endif
}

bool GetDirectoryListFromDelimitedString(const std::string& str, std::vector<std::string>& dirs)
{
    using char_separator_type =  boost::char_separator<char>;
    using tokenizer_type = boost::tokenizer<
        boost::char_separator<char>, std::string::const_iterator, std::string>;
    dirs.clear();
    if (str.empty())
    {
        return false;
    }
    char_separator_type pathSep(os_pathsep().c_str());
    tokenizer_type strTok(str, pathSep);
    typename tokenizer_type::iterator strIt;
    typename tokenizer_type::iterator strEndIt = strTok.end();
    for (strIt = strTok.begin(); strIt != strEndIt; ++strIt)
    {
        dirs.push_back(*strIt);
    }
    return (!dirs.empty());
}

bool GetDirectoryListFromDelimitedString(const std::wstring& str, std::vector<std::wstring>& dirs)
{
    using char_separator_type = boost::char_separator<wchar_t>;
    using tokenizer_type = boost::tokenizer<
        boost::char_separator<wchar_t>, std::wstring::const_iterator, std::wstring>;
    dirs.clear();
    if (str.empty())
    {
        return false;
    }
    char_separator_type pathSep(wos_pathsep().c_str());
    tokenizer_type strTok(str, pathSep);
    typename tokenizer_type::iterator strIt;
    typename tokenizer_type::iterator strEndIt = strTok.end();
    for (strIt = strTok.begin(); strIt != strEndIt; ++strIt)
    {
        dirs.push_back(*strIt);
    }
    return (!dirs.empty());
}

std::string search_path(const std::string& file)
{
    if (file.empty())
    {
        return "";
    }
    std::string ret;
#if (BOOST_VERSION >= 106400)
    {
        namespace bp = boost::process;
        boost::filesystem::path p = bp::search_path(file);
        ret = p.make_preferred().string();
    }
#endif
    if (!ret.empty())
    {
        return ret;
    }
    // Drat! I have to do it the hard way.
    std::string pathEnvVar = GetEnv("PATH");
    if (pathEnvVar.empty())
    {
        return "";
    }
    std::vector<std::string> pathDirs;
    bool getDirList = GetDirectoryListFromDelimitedString(pathEnvVar, pathDirs);
    if (!getDirList)
    {
        return "";
    }
    auto it = pathDirs.cbegin();
    auto itEnd = pathDirs.cend();
    for (; it != itEnd; ++it)
    {
        boost::filesystem::path p(*it);
        p /= file;
        boost::system::error_code ec;
        if (boost::filesystem::exists(p, ec) && boost::filesystem::is_regular_file(p, ec))
        {
            return p.make_preferred().string();
        }
    }
    return "";
}

std::wstring search_path(const std::wstring& file)
{
    if (file.empty())
    {
        return L"";
    }
    std::wstring ret;
#if (BOOST_VERSION >= 106400)
    {
        namespace bp = boost::process;
        boost::filesystem::path p = bp::search_path(file);
        ret = p.make_preferred().wstring();
    }
#endif
    if (!ret.empty())
    {
        return ret;
    }
    // Drat! I have to do it the hard way.
    std::wstring pathEnvVar = GetEnv(L"PATH");
    if (pathEnvVar.empty())
    {
        return L"";
    }
    std::vector<std::wstring> pathDirs;
    bool getDirList = GetDirectoryListFromDelimitedString(pathEnvVar, pathDirs);
    if (!getDirList)
    {
        return L"";
    }
    auto it = pathDirs.cbegin();
    auto itEnd = pathDirs.cend();
    for (; it != itEnd; ++it)
    {
        boost::filesystem::path p(*it);
        p /= file;
        if (boost::filesystem::exists(p) && boost::filesystem::is_regular_file(p))
        {
            return p.make_preferred().wstring();
        }
    }
    return L"";
}

std::string executable_path_fallback(const char* argv0)
{
    if (argv0 == nullptr || argv0[0] == 0)
    {
#if (BOOST_VERSION >= 106100 && BOOST_VERSION < 106400)
		boost::system::error_code ec;
		auto p = boost::dll::program_location(ec);
        if (ec.value() == boost::system::errc::success)
        {
            return p.make_preferred().string();
        }
        return "";
#else
        return "";
#endif
    }
    if (strstr(argv0, os_sep().c_str()) != nullptr)
    {
        boost::system::error_code ec;
        boost::filesystem::path p(
            boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
        if (ec.value() == boost::system::errc::success)
        {
            return p.make_preferred().string();
        }
    }
    std::string ret = search_path(argv0);
    if (!ret.empty())
    {
        return ret;
    }
    boost::system::error_code ec;
    boost::filesystem::path p(
        boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
    if (ec.value() == boost::system::errc::success)
    {
        ret = p.make_preferred().string();
    }
    return ret;
}

std::wstring executable_path_fallback(const wchar_t* argv0)
{
    if (argv0 == nullptr || argv0[0] == 0)
    {
#if (BOOST_VERSION >= 106100 && BOOST_VERSION < 106400)
		boost::system::error_code ec;
		auto p = boost::dll::program_location(ec);
        if (ec.value() == boost::system::errc::success)
        {
            return p.make_preferred().wstring();
        }
        return L"";
#else
        return L"";
#endif
    }
    if (wcsstr(argv0, wos_sep().c_str()) != nullptr)
    {
        boost::system::error_code ec;
        boost::filesystem::path p(
            boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
        if (ec.value() == boost::system::errc::success)
        {
            return p.make_preferred().wstring();
        }
    }
    std::wstring ret = search_path(argv0);
    if (!ret.empty())
    {
        return ret;
    }
    boost::system::error_code ec;
    boost::filesystem::path p(
        boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
    if (ec.value() == boost::system::errc::success)
    {
        ret = p.make_preferred().wstring();
    }
    return ret;
}

}  // namespace detail
} // namespace boost
