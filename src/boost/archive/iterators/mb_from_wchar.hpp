#ifndef BOOST_ARCHIVE_ITERATORS_MB_FROM_WCHAR_HPP
#define BOOST_ARCHIVE_ITERATORS_MB_FROM_WCHAR_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// mb_from_wchar.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/assert.hpp>
#include <cstddef> // size_t
#ifndef BOOST_NO_CWCHAR
#include <cwchar> //  mbstate_t
#endif
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::mbstate_t;
} // namespace std
#endif

#include <boost/archive/detail/utf8_codecvt_facet.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

namespace boost {
namespace archive {
namespace iterators {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// class used by text archives to translate wide strings and to char
// strings of the currently selected locale
template<class Base>    // the input iterator
class mb_from_wchar
    : public boost::iterator_adaptor<
        mb_from_wchar<Base>,
        Base,
        wchar_t,
        single_pass_traversal_tag,
        char
    >
{
    friend class boost::iterator_core_access;

    typedef typename boost::iterator_adaptor<
        mb_from_wchar<Base>,
        Base,
        wchar_t,
        single_pass_traversal_tag,
        char
    > super_t;

    typedef mb_from_wchar<Base> this_t;

    char dereference_impl() {
        if(! m_full){
            fill();
            m_full = true;
        }
        return m_buffer[m_bnext];
    }

    char dereference() const {
        return (const_cast<this_t *>(this))->dereference_impl();
    }
    // test for iterator equality
    bool equal(const mb_from_wchar<Base> & rhs) const {
        // once the value is filled, the base_reference has been incremented
        // so don't permit comparison anymore.
        return
            0 == m_bend
            && 0 == m_bnext
            && this->base_reference() == rhs.base_reference()
        ;
    }

    void fill(){
        wchar_t value = * this->base_reference();
        const wchar_t *wend;
        char *bend;
        BOOST_VERIFY(
            m_codecvt_facet.out(
                m_mbs,
                & value, & value + 1, wend,
                m_buffer, m_buffer + sizeof(m_buffer), bend
            )
            ==
            std::codecvt_base::ok
        );
        m_bnext = 0;
        m_bend = bend - m_buffer;
    }

    void increment(){
        if(++m_bnext < m_bend)
            return;
        m_bend =
        m_bnext = 0;
        ++(this->base_reference());
        m_full = false;
    }

    boost::archive::detail::utf8_codecvt_facet m_codecvt_facet;
    std::mbstate_t m_mbs;
    // buffer to handle pending characters
    char m_buffer[9 /* MB_CUR_MAX */];
    std::size_t m_bend;
    std::size_t m_bnext;
    bool m_full;

public:
    // make composible buy using templated constructor
    template<class T>
    mb_from_wchar(T start) :
        super_t(Base(static_cast< T >(start))),
        m_mbs(std::mbstate_t()),
        m_bend(0),
        m_bnext(0),
        m_full(false)
    {}
    // intel 7.1 doesn't like default copy constructor
    mb_from_wchar(const mb_from_wchar & rhs) :
        super_t(rhs.base_reference()),
        m_bend(rhs.m_bend),
        m_bnext(rhs.m_bnext),
        m_full(rhs.m_full)
    {}
};

} // namespace iterators
} // namespace archive
} // namespace boost

#endif // BOOST_ARCHIVE_ITERATORS_MB_FROM_WCHAR_HPP
