// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONVERT_HPP_VP_2004_04_28
#define BOOST_CONVERT_HPP_VP_2004_04_28

#include <boost/program_options/config.hpp>

#if !defined(BOOST_NO_STD_WSTRING)

#include <boost/detail/workaround.hpp>

#include <string>
#include <vector>
#include <locale>
// for mbstate_t
#include <cwchar>
#include <stdexcept>

#if defined(BOOST_NO_STDC_NAMESPACE)
#include <wchar.h>
namespace std
{
    using ::mbstate_t;
}    
#endif

namespace boost {

    /** Converts from local 8 bit encoding into wchar_t string using
        the specified locale facet. */
    BOOST_PROGRAM_OPTIONS_DECL std::wstring 
    from_8_bit(const std::string& s, 
               const std::codecvt<wchar_t, char, std::mbstate_t>& cvt);

    /** Converts from wchar_t string into local 8 bit encoding into using
        the specified locale facet. */
    BOOST_PROGRAM_OPTIONS_DECL std::string 
    to_8_bit(const std::wstring& s, 
             const std::codecvt<wchar_t, char, std::mbstate_t>& cvt);


    /** Converts 's', which is assumed to be in UTF8 encoding, into wide
        string. */
    BOOST_PROGRAM_OPTIONS_DECL std::wstring
    from_utf8(const std::string& s);
    
    /** Converts wide string 's' into string in UTF8 encoding. */
    BOOST_PROGRAM_OPTIONS_DECL std::string
    to_utf8(const std::wstring& s);

    /** Converts wide string 's' into local 8 bit encoding determined by
        the current locale. */
    BOOST_PROGRAM_OPTIONS_DECL std::string
    to_local_8_bit(const std::wstring& s);

    /** Converts 's', which is assumed to be in local 8 bit encoding, into wide
        string. */
    BOOST_PROGRAM_OPTIONS_DECL std::wstring
    from_local_8_bit(const std::string& s);

    namespace program_options
    {
        /** Convert the input string into internal encoding used by
            program_options. Presence of this function allows to avoid
            specializing all methods which access input on wchar_t.
        */
        BOOST_PROGRAM_OPTIONS_DECL std::string to_internal(const std::string&);
        /** @overload */
        BOOST_PROGRAM_OPTIONS_DECL std::string to_internal(const std::wstring&);

        template<class T>
        std::vector<std::string> to_internal(const std::vector<T>& s)
        {
            std::vector<std::string> result;
            for (unsigned i = 0; i < s.size(); ++i)
                result.push_back(to_internal(s[i]));            
            return result;
        }

    }


  
}

#else
#include <vector>
#include <string>
namespace boost{
   namespace program_options{
        BOOST_PROGRAM_OPTIONS_DECL std::string to_internal(const std::string&);

        template<class T>
        std::vector<std::string> to_internal(const std::vector<T>& s)
        {
            std::vector<std::string> result;
            for (unsigned i = 0; i < s.size(); ++i)
                result.push_back(to_internal(s[i]));            
            return result;
        }
   }
}
#endif
#endif
