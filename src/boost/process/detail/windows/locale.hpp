// Copyright (c) 2016 Klemens D. Morgenstern
// Copyright (c) 2008 Beman Dawes
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_LOCALE_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_LOCALE_HPP_

#include <locale>
#include <boost/core/ignore_unused.hpp>
#include <boost/winapi/file_management.hpp>
#include <boost/winapi/character_code_conversion.hpp>

namespace boost
{
namespace process
{
namespace detail
{
namespace windows
{

//copied from boost.filesystem
class windows_file_codecvt
   : public std::codecvt< wchar_t, char, std::mbstate_t >
 {
 public:
   explicit windows_file_codecvt(std::size_t refs = 0)
       : std::codecvt<wchar_t, char, std::mbstate_t>(refs) {}
 protected:

   bool do_always_noconv() const noexcept override { return false; }

   //  seems safest to assume variable number of characters since we don't
   //  actually know what codepage is active
   int do_encoding() const noexcept override { return 0; }

   std::codecvt_base::result do_in(std::mbstate_t& state,
     const char* from, const char* from_end, const char*& from_next,
     wchar_t* to, wchar_t* to_end, wchar_t*& to_next) const override
   {
     boost::ignore_unused(state);

       auto codepage =
#if !defined(BOOST_NO_ANSI_APIS)
               ::boost::winapi::AreFileApisANSI() ?
               ::boost::winapi::CP_ACP_ :
#endif
               ::boost::winapi::CP_OEMCP_;

     int count = 0;
     if ((count = ::boost::winapi::MultiByteToWideChar(codepage,
             ::boost::winapi::MB_PRECOMPOSED_, from,
       static_cast<int>(from_end - from), to, static_cast<int>(to_end - to))) == 0)
     {
       return error;  // conversion failed
     }

     from_next = from_end;
     to_next = to + count;
     *to_next = L'\0';
     return ok;
  }

   std::codecvt_base::result do_out(std::mbstate_t & state,
     const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next,
     char* to, char* to_end, char*& to_next) const override
   {
     boost::ignore_unused(state);
     auto codepage =
#if !defined(BOOST_NO_ANSI_APIS)
                   ::boost::winapi::AreFileApisANSI() ?
                       ::boost::winapi::CP_ACP_ :
#endif
                     ::boost::winapi::CP_OEMCP_;
     int count = 0;


     if ((count = ::boost::winapi::WideCharToMultiByte(codepage,
                   ::boost::winapi::WC_NO_BEST_FIT_CHARS_, from,
                  static_cast<int>(from_end - from), to, static_cast<int>(to_end - to), 0, 0)) == 0)
     {
       return error;  // conversion failed
     }

     from_next = from_end;
     to_next = to + count;
     *to_next = '\0';
     return ok;
   }

   std::codecvt_base::result do_unshift(std::mbstate_t&,
       char* /*from*/, char* /*to*/, char* & /*next*/) const override { return ok; }

   int do_length(std::mbstate_t&,
     const char* /*from*/, const char* /*from_end*/, std::size_t /*max*/) const override { return 0; }

   int do_max_length() const noexcept override { return 0; }
 };



}
}
}
}



#endif /* BOOST_PROCESS_LOCALE_HPP_ */
