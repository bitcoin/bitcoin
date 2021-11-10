//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_BOUNDARY_BOUNDARY_POINT_HPP_INCLUDED
#define BOOST_LOCALE_BOUNDARY_BOUNDARY_POINT_HPP_INCLUDED

#include <boost/locale/boundary/types.hpp>

namespace boost {
namespace locale {
namespace boundary {

    ///
    /// \addtogroup boundary
    /// @{

    ///
    /// \brief This class represents a boundary point in the text. 
    ///
    /// It represents a pair - an iterator and a rule that defines this 
    /// point.
    ///
    /// This type of object is dereference by the iterators of boundary_point_index. Using a rule()
    /// member function you can get the reason why this specific boundary point was selected. 
    ///
    /// For example, When you use a sentence boundary analysis, the (rule() & \ref sentence_term) != 0 means
    /// that this boundary point was selected because a sentence terminator (like .?!) was spotted
    /// and the (rule() & \ref sentence_sep)!=0 means that a separator like line feed or carriage
    /// return was observed.
    ///
    /// \note
    ///
    /// -   The beginning of analyzed range is always considered a boundary point and its rule is always 0.
    /// -   when using a word boundary analysis the returned rule relates to a chunk of text preceding
    ///     this point.
    ///
    /// \see
    ///
    /// -   \ref boundary_point_index
    /// -   \ref segment
    /// -   \ref segment_index
    ///
    template<typename IteratorType>
    class boundary_point  {
    public:
        ///
        /// The type of the base iterator that iterates the original text
        ///
        typedef IteratorType iterator_type;

        ///
        /// Empty default constructor
        ///
        boundary_point() : rule_(0) {}
        
        ///
        /// Create a new boundary_point using iterator \p and a rule \a r
        ///
        boundary_point(iterator_type p,rule_type r) :
            iterator_(p),
            rule_(r)
        {
        }
        ///
        /// Set an new iterator value \a i
        ///
        void iterator(iterator_type i)
        {
            iterator_ = i;
        }
        ///
        /// Set an new rule value \a r
        ///
        void rule(rule_type r)
        {
            rule_ = r;
        }
        ///
        /// Fetch an iterator
        ///
        iterator_type iterator() const 
        {
            return iterator_;
        }
        ///
        /// Fetch a rule
        ///
        rule_type rule() const
        {
            return rule_;
        }
        ///
        /// Check if two boundary points are the same
        ///
        bool operator==(boundary_point const &other) const
        {
            return iterator_ == other.iterator_ && rule_ = other.rule_;
        }
        ///
        /// Check if two boundary points are different
        ///
        bool operator!=(boundary_point const &other) const
        {
            return !(*this==other);
        }
        ///
        /// Check if the boundary point points to same location as an iterator \a other
        ///
        bool operator==(iterator_type const &other) const
        {
            return iterator_ == other;
        }
        ///
        /// Check if the boundary point points to different location from an iterator \a other
        ///
        bool operator!=(iterator_type const &other) const
        {
            return iterator_ != other;
        }

        ///
        /// Automatic cast to the iterator it represents
        ///
        operator iterator_type ()const
        {
            return iterator_;
        }

    private:
        iterator_type iterator_;
        rule_type rule_;
       
    };
    ///
    /// Check if the boundary point \a r points to same location as an iterator \a l
    ///
    template<typename BaseIterator>
    bool operator==(BaseIterator const &l,boundary_point<BaseIterator> const &r)
    {
        return r==l;
    }
    ///
    /// Check if the boundary point \a r points to different location from an iterator \a l
    ///
    template<typename BaseIterator>
    bool operator!=(BaseIterator const &l,boundary_point<BaseIterator> const &r)
    {
        return r!=l;
    }

    /// @}
    
    typedef boundary_point<std::string::const_iterator> sboundary_point;      ///< convenience typedef
    typedef boundary_point<std::wstring::const_iterator> wsboundary_point;    ///< convenience typedef
    #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    typedef boundary_point<std::u16string::const_iterator> u16sboundary_point;///< convenience typedef
    #endif
    #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    typedef boundary_point<std::u32string::const_iterator> u32sboundary_point;///< convenience typedef
    #endif
   
    typedef boundary_point<char const *> cboundary_point;                     ///< convenience typedef
    typedef boundary_point<wchar_t const *> wcboundary_point;                 ///< convenience typedef
    #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    typedef boundary_point<char16_t const *> u16cboundary_point;              ///< convenience typedef
    #endif
    #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    typedef boundary_point<char32_t const *> u32cboundary_point;              ///< convenience typedef
    #endif
    

} // boundary
} // locale
} // boost


#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
