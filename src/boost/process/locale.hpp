// Copyright (c) 2016 Klemens D. Morgenstern
// Copyright (c) 2008 Beman Dawes
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_LOCALE_HPP_
#define BOOST_PROCESS_LOCALE_HPP_

#include <system_error>
#include <boost/process/detail/config.hpp>

#if defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/locale.hpp>
# elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__) \
|| defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__HAIKU__)
#include <codecvt>
#endif

#include <locale>

namespace boost
{
namespace process
{
namespace detail
{

class codecvt_category_t : public std::error_category
{
public:
    codecvt_category_t() = default;
    const char* name() const noexcept override {return "codecvt";}
    std::string message(int ev) const override
    {
        std::string str;
        switch (ev)
        {
        case std::codecvt_base::ok:
            str = "ok";
            break;
        case std::codecvt_base::partial:
            str = "partial";
            break;
        case std::codecvt_base::error:
            str = "error";
            break;
        case std::codecvt_base::noconv:
            str = "noconv";
            break;
        default:
            str = "unknown error";
        }
        return str;
    }
};

}

///Internally used error cateory for code conversion.
inline const std::error_category& codecvt_category()
{
    static const ::boost::process::detail::codecvt_category_t cat;
    return cat;
}

namespace detail
{
//copied from boost.filesystem
inline std::locale default_locale()
{
# if defined(BOOST_WINDOWS_API)
    std::locale global_loc = std::locale();
    return std::locale(global_loc, new boost::process::detail::windows::windows_file_codecvt);
# elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__) \
|| defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__HAIKU__)
    std::locale global_loc = std::locale();
    return std::locale(global_loc, new std::codecvt_utf8<wchar_t>);
# else  // Other POSIX
    // ISO C calls std::locale("") "the locale-specific native environment", and this
    // locale is the default for many POSIX-based operating systems such as Linux.
    return std::locale("");
# endif
}

inline std::locale& process_locale()
{
    static std::locale loc(default_locale());
    return loc;
}

}

///The internally used type for code conversion.
typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt_type;

///Get a reference to the currently used code converter.
inline const codecvt_type& codecvt()
{
  return std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(
                detail::process_locale());
}

///Set the locale of the library.
inline std::locale imbue(const std::locale& loc)
{
  std::locale temp(detail::process_locale());
  detail::process_locale() = loc;
  return temp;
}


namespace detail
{

inline std::size_t convert(const char* from,
                    const char* from_end,
                    wchar_t* to, wchar_t* to_end,
                    const ::boost::process::codecvt_type & cvt =
                                 ::boost::process::codecvt())
{
    std::mbstate_t state  = std::mbstate_t();  // perhaps unneeded, but cuts bug reports
    const char* from_next;
    wchar_t* to_next;

    auto res = cvt.in(state, from, from_end, from_next,
                 to, to_end, to_next);

    if (res != std::codecvt_base::ok)
         throw process_error(res, ::boost::process::codecvt_category(),
             "boost::process codecvt to wchar_t");


    return to_next - to;

}

inline std::size_t convert(const wchar_t* from,
                    const wchar_t* from_end,
                    char* to, char* to_end,
                    const ::boost::process::codecvt_type & cvt =
                                 ::boost::process::codecvt())
{
    std::mbstate_t state  = std::mbstate_t();  // perhaps unneeded, but cuts bug reports
    const wchar_t* from_next;
    char* to_next;

    std::codecvt_base::result res;

    if ((res=cvt.out(state, from, from_end, from_next,
           to, to_end, to_next)) != std::codecvt_base::ok)
               throw process_error(res, ::boost::process::codecvt_category(),
                   "boost::process codecvt to char");

    return to_next - to;
}

inline std::wstring convert(const std::string & st,
                            const ::boost::process::codecvt_type & cvt =
                                ::boost::process::codecvt())
{
    std::wstring out(st.size() + 10, ' '); //just to be sure
    auto sz = convert(st.c_str(), st.c_str() + st.size(),
                      &out.front(), &out.back(), cvt);

    out.resize(sz);
    return out;
}

inline std::string convert(const std::wstring & st,
                           const ::boost::process::codecvt_type & cvt =
                                ::boost::process::codecvt())
{
    std::string out(st.size() * 2, ' '); //just to be sure
    auto sz = convert(st.c_str(), st.c_str() + st.size(),
                      &out.front(), &out.back(), cvt);

    out.resize(sz);
    return out;
}

inline std::vector<wchar_t> convert(const std::vector<char> & st,
                                    const ::boost::process::codecvt_type & cvt =
                                        ::boost::process::codecvt())
{
    std::vector<wchar_t> out(st.size() + 10); //just to be sure
    auto sz = convert(st.data(), st.data() + st.size(),
                      &out.front(), &out.back(), cvt);

    out.resize(sz);
    return out;
}

inline std::vector<char> convert(const std::vector<wchar_t> & st,
                                 const ::boost::process::codecvt_type & cvt =
                                     ::boost::process::codecvt())
{
    std::vector<char> out(st.size() * 2); //just to be sure
    auto sz = convert(st.data(), st.data() + st.size(),
                      &out.front(), &out.back(), cvt);

    out.resize(sz);
    return out;
}


inline std::wstring convert(const char *begin, const char* end,
                            const ::boost::process::codecvt_type & cvt =
                                ::boost::process::codecvt())
{
    auto size = end-begin;
    std::wstring out(size + 10, ' '); //just to be sure
    using namespace std;
    auto sz = convert(begin, end,
                      &out.front(), &out.back(), cvt);
    out.resize(sz);
    return out;
}

inline std::string convert(const wchar_t  * begin, const wchar_t *end,
                           const ::boost::process::codecvt_type & cvt =
                                ::boost::process::codecvt())
{
    auto size = end-begin;

    std::string out(size * 2, ' '); //just to be sure
    auto sz = convert(begin, end ,
                      &out.front(), &out.back(), cvt);

    out.resize(sz);
    return out;
}




}



}
}




#endif /* BOOST_PROCESS_LOCALE_HPP_ */
