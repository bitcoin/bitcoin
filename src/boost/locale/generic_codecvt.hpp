//
//  Copyright (c) 2015 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_GENERIC_CODECVT_HPP
#define BOOST_LOCALE_GENERIC_CODECVT_HPP

#include <boost/locale/utf.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <locale>

namespace boost {
namespace locale {

#ifndef BOOST_LOCALE_DOXYGEN
//
// Make sure that mbstate can keep 16 bit of UTF-16 sequence
//
BOOST_STATIC_ASSERT(sizeof(std::mbstate_t)>=2);
#endif

#if defined(_MSC_VER) && _MSC_VER < 1700
// up to MSVC 11 (2012) do_length is non-standard it counts wide characters instead of narrow and does not change mbstate
#define BOOST_LOCALE_DO_LENGTH_MBSTATE_CONST
#endif

///
/// \brief A base class that used to define constants for generic_codecvt
///
class generic_codecvt_base {
public:
    ///
    /// Initail state for converting to or from unicode code points, used by initial_state in derived classes
    ///
    enum initial_convertion_state {
        to_unicode_state, ///< The state would be used by to_unicode functions
        from_unicode_state ///< The state would be used by from_unicode functions
    };
};

///
/// \brief Geneneric generic codecvt facet, various stateless encodings to UTF-16 and UTF-32 using wchar_t, char32_t and char16_t
///
/// Implementations should dervide from this class defining itself as CodecvtImpl and provide following members
///
/// - `state_type` - a type of special object that allows to store intermediate cached data, for example `iconv_t` descriptor 
/// - `state_type initial_state(generic_codecvt_base::initial_convertion_state direction) const` - member function that creates initial state
/// - `int max_encoding_length() const` - a maximal length that one Unicode code point is represented, for UTF-8 for example it is 4 from ISO-8859-1 it is 1
/// - `utf::code_point to_unicode(state_type &state,char const *&begin,char const *end)` - extract first code point from the text in range [begin,end), in case of success begin would point to the next character sequence to be encoded to next code point, in case of incomplete sequence - utf::incomplete shell be returned, and in case of invalid input sequence utf::illegal shell be returned and begin would remain unmodified
/// - `utf::code_point from_unicode(state_type &state,utf::code_point u,char *begin,char const *end)` - convert a unicode code point `u` into a character seqnece at [begin,end). Return the length of the sequence in case of success, utf::incomplete in case of not enough room to encode the code point of utf::illegal in case conversion can not be performed
///
///
/// For example implementaion of codecvt for latin1/ISO-8859-1 character set
///
/// \code
///
/// template<typename CharType>
/// class latin1_codecvt :boost::locale::generic_codecvt<CharType,latin1_codecvt<CharType> > 
/// {
/// public:
///    
///     /* Standard codecvt constructor */ 
///     latin1_codecvt(size_t refs = 0) : boost::locale::generic_codecvt<CharType,latin1_codecvt<CharType> >(refs) 
///     {
///     }
///
///     /* State is unused but required by generic_codecvt */
///     struct state_type {};
///
///     state_type initial_state(generic_codecvt_base::initial_convertion_state /*unused*/) const
///     {
///         return state_type();
///     }
///     
///     int max_encoding_length() const
///     {
///         return 1;
///     }
///
///     boost::locale::utf::code_point to_unicode(state_type &,char const *&begin,char const *end) const
///     {
///        if(begin == end)
///           return boost::locale::utf::incomplete;
///        return *begin++; 
///     }
///
///     boost::locale::utf::code_point from_unicode(state_type &,boost::locale::utf::code_point u,char *begin,char const *end) const
///     {
///        if(u >= 256)
///           return boost::locale::utf::illegal;
///        if(begin == end)
///           return boost::locale::utf::incomplete;
///        *begin = u;
///        return 1; 
///     }
/// };
/// 
/// \endcode
/// 
/// When external tools used for encoding conversion, the `state_type` is useful to save objects used for conversions. For example,
/// icu::UConverter can be saved in such a state for an efficient use:
///
/// \code
/// template<typename CharType>
/// class icu_codecvt :boost::locale::generic_codecvt<CharType,icu_codecvt<CharType> > 
/// {
/// public:
///    
///     /* Standard codecvt constructor */ 
///     icu_codecvt(std::string const &name,refs = 0) : 
///         boost::locale::generic_codecvt<CharType,latin1_codecvt<CharType> >(refs)
///     { ... }
///
///     /* State is unused but required by generic_codecvt */
///     struct std::unique_ptr<UConverter,void (*)(UConverter*)> state_type;
///
///     state_type &&initial_state(generic_codecvt_base::initial_convertion_state /*unused*/) const
///     {
///         UErrorCode err = U_ZERO_ERROR;
///         state_type ptr(ucnv_safeClone(converter_,0,0,&err,ucnv_close);
///         return std::move(ptr);
///     }
///     
///     boost::locale::utf::code_point to_unicode(state_type &ptr,char const *&begin,char const *end) const
///     {
///         UErrorCode err = U_ZERO_ERROR;
///         boost::locale::utf::code_point cp = ucnv_getNextUChar(ptr.get(),&begin,end,&err);
///         ...
///     }
///     ...
/// };
/// \endcode
///
///
template<typename CharType,typename CodecvtImpl,int CharSize=sizeof(CharType)>
class generic_codecvt;

///
/// \brief UTF-16 to/from UTF-8 codecvt facet to use with char16_t or wchar_t on Windows
///
/// Note in order to fit the requirements of usability by std::wfstream it uses mbstate_t
/// to handle intermediate states in handling of variable length UTF-16 sequences
///
/// Its member functions implement standard virtual functions of basic codecvt
///
template<typename CharType,typename CodecvtImpl>
class generic_codecvt<CharType,CodecvtImpl,2> : public std::codecvt<CharType,char,std::mbstate_t>, public generic_codecvt_base
{
public:

    typedef CharType uchar;

    generic_codecvt(size_t refs = 0) : 
        std::codecvt<CharType,char,std::mbstate_t>(refs)
    {
    }
    CodecvtImpl const &implementation() const
    {
        return *static_cast<CodecvtImpl const *>(this);
    }

protected:


    virtual std::codecvt_base::result do_unshift(std::mbstate_t &s,char *from,char * /*to*/,char *&next) const
    {
        boost::uint16_t &state = *reinterpret_cast<boost::uint16_t *>(&s);
#ifdef DEBUG_CODECVT            
        std::cout << "Entering unshift " << std::hex << state << std::dec << std::endl;
#endif            
        if(state != 0)
            return std::codecvt_base::error;
        next=from;
        return std::codecvt_base::ok;
    }
    virtual int do_encoding() const throw()
    {
        return 0;
    }
    virtual int do_max_length() const throw()
    {
        return implementation().max_encoding_length();
    }
    virtual bool do_always_noconv() const throw()
    {
        return false;
    }

    virtual int
    do_length(  std::mbstate_t 
    #ifdef BOOST_LOCALE_DO_LENGTH_MBSTATE_CONST
            const   
    #endif
            &std_state,
            char const *from,
            char const *from_end,
            size_t max) const
    {
        #ifndef BOOST_LOCALE_DO_LENGTH_MBSTATE_CONST
        char const *save_from = from;
        boost::uint16_t &state = *reinterpret_cast<boost::uint16_t *>(&std_state);
        #else
        size_t save_max = max;
        boost::uint16_t state = *reinterpret_cast<boost::uint16_t const *>(&std_state);
        #endif

        typedef typename CodecvtImpl::state_type state_type;
        state_type cvt_state = implementation().initial_state(generic_codecvt_base::to_unicode_state);
        while(max > 0 && from < from_end){
            char const *prev_from = from;
            boost::uint32_t ch=implementation().to_unicode(cvt_state,from,from_end);
            if(ch==boost::locale::utf::incomplete || ch==boost::locale::utf::illegal) {
                from = prev_from;
                break;
            }
            max --;
            if(ch > 0xFFFF) {
                if(state == 0) {
                    from = prev_from;
                    state = 1;
                }
                else {
                    state = 0;
                }
            }        
        }
        #ifndef BOOST_LOCALE_DO_LENGTH_MBSTATE_CONST
        return from - save_from;
        #else
        return save_max - max;
        #endif
    }

    
    virtual std::codecvt_base::result 
    do_in(  std::mbstate_t &std_state,
            char const *from,
            char const *from_end,
            char const *&from_next,
            uchar *to,
            uchar *to_end,
            uchar *&to_next) const
    {
        std::codecvt_base::result r=std::codecvt_base::ok;
        
        // mbstate_t is POD type and should be initialized to 0 (i.a. state = stateT())
        // according to standard. We use it to keep a flag 0/1 for surrogate pair writing
        //
        // if 0 no code above >0xFFFF observed, of 1 a code above 0xFFFF observerd
        // and first pair is written, but no input consumed
        boost::uint16_t &state = *reinterpret_cast<boost::uint16_t *>(&std_state);
        typedef typename CodecvtImpl::state_type state_type;
        state_type cvt_state = implementation().initial_state(generic_codecvt_base::to_unicode_state);
        while(to < to_end && from < from_end)
        {
#ifdef DEBUG_CODECVT            
            std::cout << "Entering IN--------------" << std::endl;
            std::cout << "State " << std::hex << state <<std::endl;
            std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif           
            char const *from_saved = from;
            
            uint32_t ch=implementation().to_unicode(cvt_state,from,from_end);
            
            if(ch==boost::locale::utf::illegal) {
                from = from_saved;
                r=std::codecvt_base::error;
                break;
            }
            if(ch==boost::locale::utf::incomplete) {
                from = from_saved;
                r=std::codecvt_base::partial;
                break;
            }
            // Normal codepoints go direcly to stream
            if(ch <= 0xFFFF) {
                *to++=ch;
            }
            else {
                // for  other codepoints we do following
                //
                // 1. We can't consume our input as we may find ourselfs
                //    in state where all input consumed but not all output written,i.e. only
                //    1st pair is written
                // 2. We only write first pair and mark this in the state, we also revert back
                //    the from pointer in order to make sure this codepoint would be read
                //    once again and then we would consume our input together with writing
                //    second surrogate pair
                ch-=0x10000;
                boost::uint16_t vh = ch >> 10;
                boost::uint16_t vl = ch & 0x3FF;
                boost::uint16_t w1 = vh + 0xD800;
                boost::uint16_t w2 = vl + 0xDC00;
                if(state == 0) {
                    from = from_saved;
                    *to++ = w1;
                    state = 1;
                }
                else {
                    *to++ = w2;
                    state = 0;
                }
            }
        }
        from_next=from;
        to_next=to;
        if(r == std::codecvt_base::ok && (from!=from_end || state!=0))
            r = std::codecvt_base::partial;
#ifdef DEBUG_CODECVT            
        std::cout << "Returning ";
        switch(r) {
        case std::codecvt_base::ok:
            std::cout << "ok" << std::endl;
            break;
        case std::codecvt_base::partial:
            std::cout << "partial" << std::endl;
            break;
        case std::codecvt_base::error:
            std::cout << "error" << std::endl;
            break;
        default:
            std::cout << "other" << std::endl;
            break;
        }
        std::cout << "State " << std::hex << state <<std::endl;
        std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
        return r;
    }
    
    virtual std::codecvt_base::result 
    do_out( std::mbstate_t &std_state,
            uchar const *from,
            uchar const *from_end,
            uchar const *&from_next,
            char *to,
            char *to_end,
            char *&to_next) const
    {
        std::codecvt_base::result r=std::codecvt_base::ok;
        // mbstate_t is POD type and should be initialized to 0 (i.a. state = stateT())
        // according to standard. We assume that sizeof(mbstate_t) >=2 in order
        // to be able to store first observerd surrogate pair
        //
        // State: state!=0 - a first surrogate pair was observerd (state = first pair),
        // we expect the second one to come and then zero the state
        ///
        boost::uint16_t &state = *reinterpret_cast<boost::uint16_t *>(&std_state);
        typedef typename CodecvtImpl::state_type state_type;
        state_type cvt_state = implementation().initial_state(generic_codecvt_base::from_unicode_state);
        while(to < to_end && from < from_end)
        {
#ifdef DEBUG_CODECVT            
        std::cout << "Entering OUT --------------" << std::endl;
        std::cout << "State " << std::hex << state <<std::endl;
        std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
            boost::uint32_t ch=0;
            if(state != 0) {
                // if the state idecates that 1st surrogate pair was written
                // we should make sure that the second one that comes is actually
                // second surrogate
                boost::uint16_t w1 = state;
                boost::uint16_t w2 = *from; 
                // we don't forward from as writing may fail to incomplete or
                // partial conversion
                if(0xDC00 <= w2 && w2<=0xDFFF) {
                    boost::uint16_t vh = w1 - 0xD800;
                    boost::uint16_t vl = w2 - 0xDC00;
                    ch=((uint32_t(vh) << 10)  | vl) + 0x10000;
                }
                else {
                    // Invalid surrogate
                    r=std::codecvt_base::error;
                    break;
                }
            }
            else {
                ch = *from;
                if(0xD800 <= ch && ch<=0xDBFF) {
                    // if this is a first surrogate pair we put
                    // it into the state and consume it, note we don't
                    // go forward as it should be illegal so we increase
                    // the from pointer manually
                    state = ch;
                    from++;
                    continue;
                }
                else if(0xDC00 <= ch && ch<=0xDFFF) {
                    // if we observe second surrogate pair and 
                    // first only may be expected we should break from the loop with error
                    // as it is illegal input
                    r=std::codecvt_base::error;
                    break;
                }
            }
            if(!boost::locale::utf::is_valid_codepoint(ch)) {
                r=std::codecvt_base::error;
                break;
            }
            boost::uint32_t len = implementation().from_unicode(cvt_state,ch,to,to_end);
            if(len == boost::locale::utf::incomplete) {
                r=std::codecvt_base::partial;
                break;
            }
            else if(len == boost::locale::utf::illegal) {
                r=std::codecvt_base::error;
                break;
            }
            else
                    to+= len;
            state = 0;
            from++;
        }
        from_next=from;
        to_next=to;
        if(r==std::codecvt_base::ok && from!=from_end)
            r = std::codecvt_base::partial;
#ifdef DEBUG_CODECVT            
        std::cout << "Returning ";
        switch(r) {
        case std::codecvt_base::ok:
            std::cout << "ok" << std::endl;
            break;
        case std::codecvt_base::partial:
            std::cout << "partial" << std::endl;
            break;
        case std::codecvt_base::error:
            std::cout << "error" << std::endl;
            break;
        default:
            std::cout << "other" << std::endl;
            break;
        }
        std::cout << "State " << std::hex << state <<std::endl;
        std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
        return r;
    }
    
};

///
/// \brief UTF-32 to/from UTF-8 codecvt facet to use with char32_t or wchar_t on POSIX platforms
///
/// Its member functions implement standard virtual functions of basic codecvt.
/// mbstate_t is not used for UTF-32 handling due to fixed length encoding
///
template<typename CharType,typename CodecvtImpl>
class generic_codecvt<CharType,CodecvtImpl,4> : public std::codecvt<CharType,char,std::mbstate_t>, public generic_codecvt_base
{
public:
    typedef CharType uchar;

    generic_codecvt(size_t refs = 0) : 
        std::codecvt<CharType,char,std::mbstate_t>(refs)
    {
    }
    
    CodecvtImpl const &implementation() const
    {
        return *static_cast<CodecvtImpl const *>(this);
    }
    
protected:

    virtual std::codecvt_base::result do_unshift(std::mbstate_t &/*s*/,char *from,char * /*to*/,char *&next) const
    {
        next=from;
        return std::codecvt_base::ok;
    }
    virtual int do_encoding() const throw()
    {
        return 0;
    }
    virtual int do_max_length() const throw()
    {
        return implementation().max_encoding_length();
    }
    virtual bool do_always_noconv() const throw()
    {
        return false;
    }

    virtual int
    do_length(  std::mbstate_t 
    #ifdef BOOST_LOCALE_DO_LENGTH_MBSTATE_CONST
            const   
    #endif    
            &/*state*/,
            char const *from,
            char const *from_end,
            size_t max) const
    {
        #ifndef BOOST_LOCALE_DO_LENGTH_MBSTATE_CONST 
        char const *start_from = from;
        #else
        size_t save_max = max;
        #endif
        typedef typename CodecvtImpl::state_type state_type;
        state_type cvt_state = implementation().initial_state(generic_codecvt_base::to_unicode_state);
        while(max > 0 && from < from_end){
            char const *save_from = from;
            boost::uint32_t ch=implementation().to_unicode(cvt_state,from,from_end);
            if(ch==boost::locale::utf::incomplete || ch==boost::locale::utf::illegal) {
                from = save_from;
                break;
            }
            max--;
        }
        #ifndef BOOST_LOCALE_DO_LENGTH_MBSTATE_CONST 
        return from - start_from;
        #else
        return save_max - max;
        #endif
    }

    
    virtual std::codecvt_base::result 
    do_in(  std::mbstate_t &/*state*/,
            char const *from,
            char const *from_end,
            char const *&from_next,
            uchar *to,
            uchar *to_end,
            uchar *&to_next) const
    {
        std::codecvt_base::result r=std::codecvt_base::ok;
        
        // mbstate_t is POD type and should be initialized to 0 (i.a. state = stateT())
        // according to standard. We use it to keep a flag 0/1 for surrogate pair writing
        //
        // if 0 no code above >0xFFFF observed, of 1 a code above 0xFFFF observerd
        // and first pair is written, but no input consumed
        typedef typename CodecvtImpl::state_type state_type;
        state_type cvt_state = implementation().initial_state(generic_codecvt_base::to_unicode_state);
        while(to < to_end && from < from_end)
        {
#ifdef DEBUG_CODECVT            
            std::cout << "Entering IN--------------" << std::endl;
            std::cout << "State " << std::hex << state <<std::endl;
            std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif           
            char const *from_saved = from;
            
            uint32_t ch=implementation().to_unicode(cvt_state,from,from_end);
            
            if(ch==boost::locale::utf::illegal) {
                r=std::codecvt_base::error;
                from = from_saved;
                break;
            }
            if(ch==boost::locale::utf::incomplete) {
                r=std::codecvt_base::partial;
                from=from_saved;
                break;
            }
            *to++=ch;
        }
        from_next=from;
        to_next=to;
        if(r == std::codecvt_base::ok && from!=from_end)
            r = std::codecvt_base::partial;
#ifdef DEBUG_CODECVT            
        std::cout << "Returning ";
        switch(r) {
        case std::codecvt_base::ok:
            std::cout << "ok" << std::endl;
            break;
        case std::codecvt_base::partial:
            std::cout << "partial" << std::endl;
            break;
        case std::codecvt_base::error:
            std::cout << "error" << std::endl;
            break;
        default:
            std::cout << "other" << std::endl;
            break;
        }
        std::cout << "State " << std::hex << state <<std::endl;
        std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
        return r;
    }
    
    virtual std::codecvt_base::result 
    do_out( std::mbstate_t &/*std_state*/,
            uchar const *from,
            uchar const *from_end,
            uchar const *&from_next,
            char *to,
            char *to_end,
            char *&to_next) const
    {
        std::codecvt_base::result r=std::codecvt_base::ok;
        typedef typename CodecvtImpl::state_type state_type;
        state_type cvt_state = implementation().initial_state(generic_codecvt_base::from_unicode_state);
        while(to < to_end && from < from_end)
        {
#ifdef DEBUG_CODECVT            
        std::cout << "Entering OUT --------------" << std::endl;
        std::cout << "State " << std::hex << state <<std::endl;
        std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
            boost::uint32_t ch=0;
            ch = *from;
            if(!boost::locale::utf::is_valid_codepoint(ch)) {
                r=std::codecvt_base::error;
                break;
            }
            boost::uint32_t len = implementation().from_unicode(cvt_state,ch,to,to_end);
            if(len == boost::locale::utf::incomplete) {
                r=std::codecvt_base::partial;
                break;
            }
            else if(len == boost::locale::utf::illegal) {
                r=std::codecvt_base::error;
                break;
            }
            to+=len;
            from++;
        }
        from_next=from;
        to_next=to;
        if(r==std::codecvt_base::ok && from!=from_end)
            r = std::codecvt_base::partial;
#ifdef DEBUG_CODECVT            
        std::cout << "Returning ";
        switch(r) {
        case std::codecvt_base::ok:
            std::cout << "ok" << std::endl;
            break;
        case std::codecvt_base::partial:
            std::cout << "partial" << std::endl;
            break;
        case std::codecvt_base::error:
            std::cout << "error" << std::endl;
            break;
        default:
            std::cout << "other" << std::endl;
            break;
        }
        std::cout << "State " << std::hex << state <<std::endl;
        std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
        return r;
    }
};


template<typename CharType,typename CodecvtImpl>
class generic_codecvt<CharType,CodecvtImpl,1> : public std::codecvt<CharType,char,std::mbstate_t>, public generic_codecvt_base
{
public:
    typedef CharType uchar;

    CodecvtImpl const &implementation() const
    {
        return *static_cast<CodecvtImpl const *>(this);
    }

    generic_codecvt(size_t refs = 0) :  std::codecvt<char,char,std::mbstate_t>(refs)
    {
    }
};

} // locale
} // namespace boost

#endif
///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
