// ----------------------------------------------------------------------------
//  format_class.hpp :  class interface
// ----------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ----------------------------------------------------------------------------

#ifndef BOOST_FORMAT_CLASS_HPP
#define BOOST_FORMAT_CLASS_HPP


#include <vector>
#include <string>

#include <boost/optional.hpp> // to store locale when needed

#include <boost/format/format_fwd.hpp>
#include <boost/format/internals_fwd.hpp>
#include <boost/format/internals.hpp>
#include <boost/format/alt_sstream.hpp>

namespace boost {

    template<class Ch, class Tr, class Alloc>
    class basic_format 
    {
        typedef typename io::CompatTraits<Tr>::compatible_type compat_traits;  
    public:
        typedef Ch  CharT;   // borland fails in operator% if we use Ch and Tr directly
        typedef std::basic_string<Ch, Tr, Alloc>              string_type;
        typedef typename string_type::size_type               size_type;
        typedef io::detail::format_item<Ch, Tr, Alloc>        format_item_t;
        typedef io::basic_altstringbuf<Ch, Tr, Alloc>         internal_streambuf_t;
        

        explicit basic_format(const Ch* str=NULL);
        explicit basic_format(const string_type& s);
        basic_format(const basic_format& x);
        basic_format& operator= (const basic_format& x);
        void swap(basic_format& x);

#if !defined(BOOST_NO_STD_LOCALE)
        explicit basic_format(const Ch* str, const std::locale & loc);
        explicit basic_format(const string_type& s, const std::locale & loc);
#endif
        io::detail::locale_t  getloc() const;

        basic_format& clear();       // empty all converted string buffers (except bound items)
        basic_format& clear_binds(); // unbind all bound items, and call clear()
        basic_format& parse(const string_type&); // resets buffers and parse a new format string

        // ** formatted result ** //
        size_type   size() const;    // sum of the current string pieces sizes
        string_type str()  const;    // final string 

        // ** arguments passing ** //
        template<class T>  
        basic_format&   operator%(const T& x)
            { return io::detail::feed<CharT, Tr, Alloc, const T&>(*this,x); }

#ifndef BOOST_NO_OVERLOAD_FOR_NON_CONST
        template<class T>  basic_format&   operator%(T& x) 
            { return io::detail::feed<CharT, Tr, Alloc, T&>(*this,x); }
#endif

        template<class T>
        basic_format& operator%(volatile const T& x)
            { /* make a non-volatile copy */ const T v(x);
              /* pass the copy along      */ return io::detail::feed<CharT, Tr, Alloc, const T&>(*this, v); }

#ifndef BOOST_NO_OVERLOAD_FOR_NON_CONST
        template<class T>
        basic_format& operator%(volatile T& x)
            { /* make a non-volatile copy */ T v(x);
              /* pass the copy along      */ return io::detail::feed<CharT, Tr, Alloc, T&>(*this, v); }
#endif

#if defined(__GNUC__)
        // GCC can't handle anonymous enums without some help
        // ** arguments passing ** //
        basic_format&   operator%(const int& x)
            { return io::detail::feed<CharT, Tr, Alloc, const int&>(*this,x); }

#ifndef BOOST_NO_OVERLOAD_FOR_NON_CONST
        basic_format&   operator%(int& x)
            { return io::detail::feed<CharT, Tr, Alloc, int&>(*this,x); }
#endif
#endif

        // The total number of arguments expected to be passed to the format objectt
        int expected_args() const
            { return num_args_; }
        // The number of arguments currently bound (see bind_arg(..) )
        int bound_args() const;
        // The number of arguments currently fed to the format object
        int fed_args() const;
        // The index (1-based) of the current argument (i.e. next to be formatted)
        int cur_arg() const;
        // The number of arguments still required to be fed
        int remaining_args() const; // same as expected_args() - bound_args() - fed_args()


        // ** object modifying **//
        template<class T>
        basic_format&  bind_arg(int argN, const T& val) 
            { return io::detail::bind_arg_body(*this, argN, val); }
        basic_format&  clear_bind(int argN);
        template<class T> 
        basic_format&  modify_item(int itemN, T manipulator) 
            { return io::detail::modify_item_body<Ch,Tr, Alloc, T> (*this, itemN, manipulator);}

        // Choosing which errors will throw exceptions :
        unsigned char exceptions() const;
        unsigned char exceptions(unsigned char newexcept);

#if !defined( BOOST_NO_MEMBER_TEMPLATE_FRIENDS )  \
    && !BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x570) \
    && !BOOST_WORKAROUND( _CRAYC, != 0) \
    && !BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))
        // use friend templates and private members only if supported

#ifndef  BOOST_NO_TEMPLATE_STD_STREAM
        template<class Ch2, class Tr2, class Alloc2>
        friend std::basic_ostream<Ch2, Tr2> & 
        operator<<( std::basic_ostream<Ch2, Tr2> & ,
                    const basic_format<Ch2, Tr2, Alloc2>& );
#else
        template<class Ch2, class Tr2, class Alloc2>
        friend std::ostream & 
        operator<<( std::ostream & ,
                    const basic_format<Ch2, Tr2, Alloc2>& );
#endif

        template<class Ch2, class Tr2, class Alloc2, class T>  
        friend basic_format<Ch2, Tr2, Alloc2>&  
        io::detail::feed_impl (basic_format<Ch2, Tr2, Alloc2>&, T);

        template<class Ch2, class Tr2, class Alloc2, class T>  friend   
        void io::detail::distribute (basic_format<Ch2, Tr2, Alloc2>&, T);
        
        template<class Ch2, class Tr2, class Alloc2, class T>  friend
        basic_format<Ch2, Tr2, Alloc2>& 
        io::detail::modify_item_body (basic_format<Ch2, Tr2, Alloc2>&, int, T);
        
        template<class Ch2, class Tr2, class Alloc2, class T> friend
        basic_format<Ch2, Tr2, Alloc2>&  
        io::detail::bind_arg_body (basic_format<Ch2, Tr2, Alloc2>&, int, const T&);

    private:
#endif
        typedef io::detail::stream_format_state<Ch, Tr>  stream_format_state;
        // flag bits, used for style_
        enum style_values  { ordered = 1, // set only if all directives are  positional
                             special_needs = 4 };     

        void make_or_reuse_data(std::size_t nbitems);// used for (re-)initialisation

        // member data --------------------------------------------//
        std::vector<format_item_t>  items_; // each '%..' directive leads to a format_item
        std::vector<bool> bound_; // stores which arguments were bound. size() == 0 || num_args

        int              style_; // style of format-string :  positional or not, etc
        int             cur_arg_; // keep track of wich argument is current
        int            num_args_; // number of expected arguments
        mutable bool     dumped_; // true only after call to str() or <<
        string_type      prefix_; // piece of string to insert before first item
        unsigned char exceptions_;
        internal_streambuf_t   buf_; // the internal stream buffer.
        boost::optional<io::detail::locale_t>     loc_;
    }; // class basic_format

} // namespace boost


#endif // BOOST_FORMAT_CLASS_HPP
