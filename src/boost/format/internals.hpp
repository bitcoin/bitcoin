// ----------------------------------------------------------------------------
// internals.hpp :  internal structs : stream_format_state, format_item. 
//                  included by format.hpp
// ----------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ----------------------------------------------------------------------------

#ifndef BOOST_FORMAT_INTERNALS_HPP
#define BOOST_FORMAT_INTERNALS_HPP


#include <string>
#include <boost/assert.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/optional.hpp>
#include <boost/limits.hpp>
#include <boost/format/detail/compat_workarounds.hpp>
#include <boost/format/alt_sstream.hpp> // used as a dummy stream

namespace boost {
namespace io {
namespace detail {


//---- stream_format_state --------------------------------------------------//

//   set of params that define the format state of a stream
    template<class Ch, class Tr> 
    struct stream_format_state 
    {
        typedef BOOST_IO_STD basic_ios<Ch, Tr>   basic_ios;

        stream_format_state(Ch fill)                 { reset(fill); }
//        stream_format_state(const basic_ios& os)     { set_by_stream(os); }

        void reset(Ch fill);                     //- sets to default state.
        void set_by_stream(const basic_ios& os); //- sets to os's state.
        void apply_on(basic_ios & os,            //- applies format_state to the stream
                      boost::io::detail::locale_t * loc_default = 0) const;
        template<class T> 
        void apply_manip(T manipulator)          //- modifies state by applying manipulator
            { apply_manip_body<Ch, Tr, T>( *this, manipulator) ; }

        // --- data ---
        std::streamsize width_;
        std::streamsize precision_;
        Ch fill_; 
        std::ios_base::fmtflags flags_;
        std::ios_base::iostate  rdstate_;
        std::ios_base::iostate  exceptions_;
        boost::optional<boost::io::detail::locale_t>  loc_;
    };  


//---- format_item  ---------------------------------------------------------//

//   stores all parameters that can be specified in format strings
    template<class Ch, class Tr, class Alloc>  
    struct format_item 
    {     
        enum pad_values { zeropad = 1, spacepad =2, centered=4, tabulation = 8 };
                         // 1. if zeropad is set, all other bits are not, 
                         // 2. if tabulation is set, all others are not.
                         // centered and spacepad can be mixed freely.
        enum arg_values { argN_no_posit   = -1, // non-positional directive. will set argN later
                          argN_tabulation = -2, // tabulation directive. (no argument read) 
                          argN_ignored    = -3  // ignored directive. (no argument read)
        };
        typedef BOOST_IO_STD basic_ios<Ch, Tr>                    basic_ios;
        typedef detail::stream_format_state<Ch, Tr>               stream_format_state;
        typedef ::std::basic_string<Ch, Tr, Alloc>                string_type;

        format_item(Ch fill) :argN_(argN_no_posit), fmtstate_(fill), 
                              truncate_(max_streamsize()), pad_scheme_(0)  {}
        void reset(Ch fill);
        void compute_states(); // sets states  according to truncate and pad_scheme.

        static std::streamsize max_streamsize() { 
            return (std::numeric_limits<std::streamsize>::max)();
        }

        // --- data ---
        int         argN_;  //- argument number (starts at 0,  eg : %1 => argN=0)
                            //  negative values for items that don't process an argument
        string_type  res_;      //- result of the formatting of this item
        string_type  appendix_; //- piece of string between this item and the next

        stream_format_state fmtstate_;// set by parsing, is only affected by modify_item

        std::streamsize truncate_;//- is set for directives like %.5s that ask truncation
        unsigned int pad_scheme_;//- several possible padding schemes can mix. see pad_values
    }; 



//--- Definitions  ------------------------------------------------------------

// -   stream_format_state:: -------------------------------------------------
    template<class Ch, class Tr>
    void stream_format_state<Ch,Tr>:: apply_on (basic_ios & os,
                      boost::io::detail::locale_t * loc_default) const {
    // If a locale is available, set it first. "os.fill(fill_);" may chrash otherwise. 
#if !defined(BOOST_NO_STD_LOCALE)
        if(loc_)
            os.imbue(loc_.get());
        else if(loc_default)
            os.imbue(*loc_default);
#else
        ignore_unused(loc_default);
#endif        
        // set the state of this stream according to our params
        if(width_ != -1)
            os.width(width_);
        if(precision_ != -1)
            os.precision(precision_);
        if(fill_ != 0)
            os.fill(fill_);
        os.flags(flags_);
        os.clear(rdstate_);
        os.exceptions(exceptions_);
    }

    template<class Ch, class Tr>
    void stream_format_state<Ch,Tr>:: set_by_stream(const basic_ios& os) {
        // set our params according to the state of this stream
        flags_ = os.flags();
        width_ = os.width();
        precision_ = os.precision();
        fill_ = os.fill();
        rdstate_ = os.rdstate();
        exceptions_ = os.exceptions();
    }


    template<class Ch, class Tr, class T>
    void apply_manip_body( stream_format_state<Ch, Tr>& self,
                           T manipulator) {
        // modify our params according to the manipulator
        basic_oaltstringstream<Ch, Tr>  ss;
        self.apply_on( ss );
        ss << manipulator;
        self.set_by_stream( ss );
    }

    template<class Ch, class Tr> inline
    void stream_format_state<Ch,Tr>:: reset(Ch fill) {
        // set our params to standard's default state.   cf 27.4.4.1 of the C++ norm
        width_=0; precision_=6; 
        fill_=fill; // default is widen(' '), but we cant compute it without the locale
        flags_ = std::ios_base::dec | std::ios_base::skipws; 
        // the adjust_field part is left equal to 0, which means right.
        exceptions_ = std::ios_base::goodbit;
        rdstate_ = std::ios_base::goodbit;
    }


// ---   format_item:: --------------------------------------------------------

    template<class Ch, class Tr, class Alloc> 
    void format_item<Ch, Tr, Alloc>:: 
    reset (Ch fill) { 
        argN_=argN_no_posit; truncate_ = max_streamsize(); pad_scheme_ =0; 
        res_.resize(0); appendix_.resize(0);
        fmtstate_.reset(fill);
    }

    template<class Ch, class Tr, class Alloc> 
    void format_item<Ch, Tr, Alloc>:: 
    compute_states() {
        // reflect pad_scheme_   on  fmt_state_
        //   because some pad_schemes has complex consequences on several state params.
        if(pad_scheme_ & zeropad) {
            // ignore zeropad in left alignment :
            if(fmtstate_.flags_ & std::ios_base::left) {
              BOOST_ASSERT(!(fmtstate_.flags_ &(std::ios_base::adjustfield ^std::ios_base::left)));
              // only left bit might be set. (not right, nor internal)
              pad_scheme_ = pad_scheme_ & (~zeropad); 
            }
            else { 
                pad_scheme_ &= ~spacepad; // printf ignores spacepad when zeropadding
                fmtstate_.fill_='0'; 
                fmtstate_.flags_ = (fmtstate_.flags_ & ~std::ios_base::adjustfield) 
                    | std::ios_base::internal;
                // removes all adjustfield bits, and adds internal.
            }
        }
        if(pad_scheme_ & spacepad) {
            if(fmtstate_.flags_ & std::ios_base::showpos)
                pad_scheme_ &= ~spacepad;
        }
    }


} } } // namespaces boost :: io :: detail


#endif // BOOST_FORMAT_INTERNALS_HPP
