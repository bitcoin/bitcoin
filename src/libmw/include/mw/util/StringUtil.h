#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Traits.h>

#include <fmt/format.h>
#include <memory>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <system_error>

#include <util/strencodings.h>

#ifdef _MSC_VER
#pragma warning(disable : 4840)
#endif

class StringUtil
{
public:
    template<typename ... Args>
    static std::string Format(const char* format, const Args& ... args)
    {
        return fmt::format(format, ConvertArg(args) ...);
    }

    static bool StartsWith(const std::string& value, const std::string& beginning)
    {
        if (beginning.size() > value.size()) {
            return false;
        }

        return std::equal(beginning.begin(), beginning.end(), value.begin());
    }

    static bool EndsWith(const std::string& value, const std::string& ending)
    {
        if (ending.size() > value.size()) {
            return false;
        }

        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter)
    {
        // Skip delimiters at beginning.
        std::string::size_type lastPos = str.find_first_not_of(delimiter, 0);

        // Find first non-delimiter.
        std::string::size_type pos = str.find_first_of(delimiter, lastPos);

        std::vector<std::string> tokens;
        while (std::string::npos != pos || std::string::npos != lastPos) {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));

            // Skip delimiters.
            lastPos = str.find_first_not_of(delimiter, pos);

            // Find next non-delimiter.
            pos = str.find_first_of(delimiter, lastPos);
        }

        return tokens;
    }

    static std::string ToLower(const std::string& str)
    {
        std::locale loc;
        std::string output = "";

        for (char elem : str) {
            output += std::tolower(elem, loc);
        }

        return output;
    }

    static std::string ToUTF8(const std::wstring& wstr)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(wstr);
    }

    static std::wstring ToWide(const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    static std::string Trim(const std::string& s)
    {
        std::string copy = s;

        // trim from start
        copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](char ch) {
            return !IsSpace(ch);
        }));

        // trim from end (in place)
        copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](char ch) {
            return !IsSpace(ch) && ch != '\r' && ch != '\n';
        }).base(), copy.end());

        return copy;
    }

private:
    static std::string ConvertArg(const std::string& x)
    {
        return x;
    }

    static std::string ConvertArg(const char* x)
    {
        return std::string(x);
    }

    static std::string ConvertArg(const std::wstring& x)
    {
        return StringUtil::ToUTF8(x);
    }

    static std::string ConvertArg(const Traits::IPrintable& x)
    {
        return x.Format();
    }

    template <class T>
    static std::string ConvertArg(const std::vector<T>& x)
    {
        std::string str;
        for (const T& value : x) {
            if (str.empty()) {
                str += ConvertArg(value);
            } else {
                str += "," + ConvertArg(value);
            }
        }

        return "[" + str + "]";
    }

    static std::string ConvertArg(const std::shared_ptr<const Traits::IPrintable>& x)
    {
        if (x == nullptr) {
            return "NULL";
        }

        return x->Format();
    }

    static std::string ConvertArg(const std::exception& e)
    {
        return std::string(e.what());
    }

    static std::string ConvertArg(const std::error_code& ec)
    {
        return std::to_string(ec.value()) + " - " + ec.message();
    }

    template <class T, typename SFINAE = std::enable_if_t<std::is_fundamental<T>::value>>
    static decltype(auto) ConvertArg(const T& x)
    {
        return x;
    }
};