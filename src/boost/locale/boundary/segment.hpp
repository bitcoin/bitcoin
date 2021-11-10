//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_BOUNDARY_SEGMENT_HPP_INCLUDED
#define BOOST_LOCALE_BOUNDARY_SEGMENT_HPP_INCLUDED
#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <locale>
#include <string>
#include <iosfwd>
#include <iterator>


namespace boost {
namespace locale {
namespace boundary {
    /// \cond INTERNAL
    namespace details {
        template<typename LeftIterator,typename RightIterator>
        int compare_text(LeftIterator l_begin,LeftIterator l_end,RightIterator r_begin,RightIterator r_end)
        {
            typedef LeftIterator left_iterator;
            typedef typename std::iterator_traits<left_iterator>::value_type char_type;
            typedef std::char_traits<char_type> traits;
            while(l_begin!=l_end && r_begin!=r_end) {
                char_type lchar = *l_begin++;
                char_type rchar = *r_begin++;
                if(traits::eq(lchar,rchar))
                    continue;
                if(traits::lt(lchar,rchar))
                    return -1;
                else
                    return 1;
            }
            if(l_begin==l_end && r_begin==r_end)
                return 0;
            if(l_begin==l_end)
                return -1;
            else
                return 1;
        }


        template<typename Left,typename Right>
        int compare_text(Left const &l,Right const &r)
        {
            return compare_text(l.begin(),l.end(),r.begin(),r.end());
        }
        
        template<typename Left,typename Char>
        int compare_string(Left const &l,Char const *begin)
        {
            Char const *end = begin;
            while(*end!=0)
                end++;
            return compare_text(l.begin(),l.end(),begin,end);
        }

        template<typename Right,typename Char>
        int compare_string(Char const *begin,Right const &r)
        {
            Char const *end = begin;
            while(*end!=0)
                end++;
            return compare_text(begin,end,r.begin(),r.end());
        }

    }
    /// \endcond

    ///
    /// \addtogroup boundary
    /// @{

    ///
    /// \brief a segment object that represents a pair of two iterators that define the range where
    /// this segment exits and a rule that defines it.
    ///
    /// This type of object is dereferenced by the iterators of segment_index. Using a rule() member function
    /// you can get a specific rule this segment was selected with. For example, when you use
    /// word boundary analysis, you can check if the specific word contains Kana letters by checking (rule() & \ref word_kana)!=0
    /// For a sentence analysis you can check if the sentence is selected because a sentence terminator is found (\ref sentence_term) or
    /// there is a line break (\ref sentence_sep).
    ///
    /// This object can be automatically converted to std::basic_string with the same type of character. It is also
    /// valid range that has begin() and end() member functions returning iterators on the location of the segment.
    ///
    /// \see
    ///
    /// - \ref segment_index
    /// - \ref boundary_point 
    /// - \ref boundary_point_index 
    ///
    template<typename IteratorType>
    class segment : public std::pair<IteratorType,IteratorType> {
    public:
        ///
        /// The type of the underlying character 
        ///
        typedef typename std::iterator_traits<IteratorType>::value_type char_type;
        ///
        /// The type of the string it is converted to
        ///
        typedef std::basic_string<char_type> string_type;
        ///
        /// The value that iterators return  - the character itself
        ///
        typedef char_type value_type;
        ///
        /// The iterator that allows to iterate the range
        ///
        typedef IteratorType iterator;
        ///
        /// The iterator that allows to iterate the range
        ///
        typedef IteratorType const_iterator;
        ///
        /// The type that represent a difference between two iterators
        ///
        typedef typename std::iterator_traits<IteratorType>::difference_type difference_type;

        ///
        /// Default constructor
        ///
        segment() {}
        ///
        /// Create a segment using two iterators and a rule that represents this point
        ///
        segment(iterator b,iterator e,rule_type r) :
            std::pair<IteratorType,IteratorType>(b,e),
            rule_(r)
        {
        }
        ///
        /// Set the start of the range
        ///
        void begin(iterator const &v)
        {
            this->first = v;
        }
        ///
        /// Set the end of the range
        ///
         void end(iterator const &v)
        {
            this->second = v;
        }

        ///
        /// Get the start of the range
        ///
        IteratorType begin() const 
        {
            return this->first;
        }
        ///
        /// Set the end of the range
        ///
        IteratorType end() const
        {
            return this->second;
        }

        ///
        /// Convert the range to a string automatically
        ///
        template <class T, class A>
        operator std::basic_string<char_type, T, A> ()const
        {
            return std::basic_string<char_type, T, A>(this->first, this->second);
        }
        
        ///
        /// Create a string from the range explicitly
        ///
        string_type str() const
        {
            return string_type(begin(),end());
        }

        ///
        /// Get the length of the text chunk
        ///

        size_t length() const
        {
            return std::distance(begin(),end());
        }

        ///
        /// Check if the segment is empty
        ///
        bool empty() const
        {
            return begin() == end();
        }

        ///
        /// Get the rule that is used for selection of this segment.
        ///
        rule_type rule() const
        {
            return rule_;
        }
        ///
        /// Set a rule that is used for segment selection
        ///
        void rule(rule_type r)
        {
            rule_ = r;
        }

        // make sure we override std::pair's operator==

        /// Compare two segments
        bool operator==(segment const &other)
        {
            return details::compare_text(*this,other) == 0;
        }

        /// Compare two segments
        bool operator!=(segment const &other)
        {
            return details::compare_text(*this,other) != 0;
        }

    private:
        rule_type rule_;
       
    };

   
    /// Compare two segments
    template<typename IteratorL,typename IteratorR>
    bool operator==(segment<IteratorL> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) == 0; 
    }
    /// Compare two segments
    template<typename IteratorL,typename IteratorR>
    bool operator!=(segment<IteratorL> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) != 0; 
    }

    /// Compare two segments
    template<typename IteratorL,typename IteratorR>
    bool operator<(segment<IteratorL> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) < 0; 
    }
    /// Compare two segments
    template<typename IteratorL,typename IteratorR>
    bool operator<=(segment<IteratorL> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) <= 0; 
    }
    /// Compare two segments
    template<typename IteratorL,typename IteratorR>
    bool operator>(segment<IteratorL> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) > 0; 
    }
    /// Compare two segments
    template<typename IteratorL,typename IteratorR>
    bool operator>=(segment<IteratorL> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) >= 0; 
    }

    /// Compare string and segment
    template<typename CharType,typename Traits,typename Alloc,typename IteratorR>
    bool operator==(std::basic_string<CharType,Traits,Alloc> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) == 0; 
    }
    /// Compare string and segment
    template<typename CharType,typename Traits,typename Alloc,typename IteratorR>
    bool operator!=(std::basic_string<CharType,Traits,Alloc> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) != 0; 
    }

    /// Compare string and segment
    template<typename CharType,typename Traits,typename Alloc,typename IteratorR>
    bool operator<(std::basic_string<CharType,Traits,Alloc> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) < 0; 
    }
    /// Compare string and segment
    template<typename CharType,typename Traits,typename Alloc,typename IteratorR>
    bool operator<=(std::basic_string<CharType,Traits,Alloc> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) <= 0; 
    }
    /// Compare string and segment
    template<typename CharType,typename Traits,typename Alloc,typename IteratorR>
    bool operator>(std::basic_string<CharType,Traits,Alloc> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) > 0; 
    }
    /// Compare string and segment
    template<typename CharType,typename Traits,typename Alloc,typename IteratorR>
    bool operator>=(std::basic_string<CharType,Traits,Alloc> const &l,segment<IteratorR> const &r)
    {
        return details::compare_text(l,r) >= 0; 
    }

    /// Compare string and segment
    template<typename Iterator,typename CharType,typename Traits,typename Alloc>
    bool operator==(segment<Iterator> const &l,std::basic_string<CharType,Traits,Alloc> const &r)
    {
        return details::compare_text(l,r) == 0; 
    }
    /// Compare string and segment
    template<typename Iterator,typename CharType,typename Traits,typename Alloc>
    bool operator!=(segment<Iterator> const &l,std::basic_string<CharType,Traits,Alloc> const &r)
    {
        return details::compare_text(l,r) != 0; 
    }

    /// Compare string and segment
    template<typename Iterator,typename CharType,typename Traits,typename Alloc>
    bool operator<(segment<Iterator> const &l,std::basic_string<CharType,Traits,Alloc> const &r)
    {
        return details::compare_text(l,r) < 0; 
    }
    /// Compare string and segment
    template<typename Iterator,typename CharType,typename Traits,typename Alloc>
    bool operator<=(segment<Iterator> const &l,std::basic_string<CharType,Traits,Alloc> const &r)
    {
        return details::compare_text(l,r) <= 0; 
    }
    /// Compare string and segment
    template<typename Iterator,typename CharType,typename Traits,typename Alloc>
    bool operator>(segment<Iterator> const &l,std::basic_string<CharType,Traits,Alloc> const &r)
    {
        return details::compare_text(l,r) > 0; 
    }
    /// Compare string and segment
    template<typename Iterator,typename CharType,typename Traits,typename Alloc>
    bool operator>=(segment<Iterator> const &l,std::basic_string<CharType,Traits,Alloc> const &r)
    {
        return details::compare_text(l,r) >= 0; 
    }


    /// Compare C string and segment
    template<typename CharType,typename IteratorR>
    bool operator==(CharType const *l,segment<IteratorR> const &r)
    {
        return details::compare_string(l,r) == 0; 
    }
    /// Compare C string and segment
    template<typename CharType,typename IteratorR>
    bool operator!=(CharType const *l,segment<IteratorR> const &r)
    {
        return details::compare_string(l,r) != 0; 
    }

    /// Compare C string and segment
    template<typename CharType,typename IteratorR>
    bool operator<(CharType const *l,segment<IteratorR> const &r)
    {
        return details::compare_string(l,r) < 0; 
    }
    /// Compare C string and segment
    template<typename CharType,typename IteratorR>
    bool operator<=(CharType const *l,segment<IteratorR> const &r)
    {
        return details::compare_string(l,r) <= 0; 
    }
    /// Compare C string and segment
    template<typename CharType,typename IteratorR>
    bool operator>(CharType const *l,segment<IteratorR> const &r)
    {
        return details::compare_string(l,r) > 0; 
    }
    /// Compare C string and segment
    template<typename CharType,typename IteratorR>
    bool operator>=(CharType const *l,segment<IteratorR> const &r)
    {
        return details::compare_string(l,r) >= 0; 
    }

    /// Compare C string and segment
    template<typename Iterator,typename CharType>
    bool operator==(segment<Iterator> const &l,CharType const *r)
    {
        return details::compare_string(l,r) == 0; 
    }
    /// Compare C string and segment
    template<typename Iterator,typename CharType>
    bool operator!=(segment<Iterator> const &l,CharType const *r)
    {
        return details::compare_string(l,r) != 0; 
    }

    /// Compare C string and segment
    template<typename Iterator,typename CharType>
    bool operator<(segment<Iterator> const &l,CharType const *r)
    {
        return details::compare_string(l,r) < 0; 
    }
    /// Compare C string and segment
    template<typename Iterator,typename CharType>
    bool operator<=(segment<Iterator> const &l,CharType const *r)
    {
        return details::compare_string(l,r) <= 0; 
    }
    /// Compare C string and segment
    template<typename Iterator,typename CharType>
    bool operator>(segment<Iterator> const &l,CharType const *r)
    {
        return details::compare_string(l,r) > 0; 
    }
    /// Compare C string and segment
    template<typename Iterator,typename CharType>
    bool operator>=(segment<Iterator> const &l,CharType const *r)
    {
        return details::compare_string(l,r) >= 0; 
    }






    typedef segment<std::string::const_iterator> ssegment;      ///< convenience typedef
    typedef segment<std::wstring::const_iterator> wssegment;    ///< convenience typedef
    #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    typedef segment<std::u16string::const_iterator> u16ssegment;///< convenience typedef
    #endif
    #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    typedef segment<std::u32string::const_iterator> u32ssegment;///< convenience typedef
    #endif
   
    typedef segment<char const *> csegment;                     ///< convenience typedef
    typedef segment<wchar_t const *> wcsegment;                 ///< convenience typedef
    #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    typedef segment<char16_t const *> u16csegment;              ///< convenience typedef
    #endif
    #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    typedef segment<char32_t const *> u32csegment;              ///< convenience typedef
    #endif



 
    
    ///
    /// Write the segment to the stream character by character
    ///
    template<typename CharType,typename TraitsType,typename Iterator>
    std::basic_ostream<CharType,TraitsType> &operator<<(
            std::basic_ostream<CharType,TraitsType> &out,
            segment<Iterator> const &tok)
    {
        for(Iterator p=tok.begin(),e=tok.end();p!=e;++p)
            out << *p;
        return out;
    }

    /// @}

} // boundary
} // locale
} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
