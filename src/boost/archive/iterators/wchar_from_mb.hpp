#ifndef BOOST_ARCHIVE_ITERATORS_WCHAR_FROM_MB_HPP
#define BOOST_ARCHIVE_ITERATORS_WCHAR_FROM_MB_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// wchar_from_mb.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cctype>
#include <cstddef> // size_t
#ifndef BOOST_NO_CWCHAR
#include <cwchar>  // mbstate_t
#endif
#include <algorithm> // copy

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::mbstate_t;
} // namespace std
#endif
#include <boost/assert.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/array.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/archive/detail/utf8_codecvt_facet.hpp>
#include <boost/archive/iterators/dataflow_exception.hpp>
#include <boost/serialization/throw_exception.hpp>

#include <iostream>

namespace boost {
namespace archive {
namespace iterators {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// class used by text archives to translate char strings to wchar_t
// strings of the currently selected locale
template<class Base>
class wchar_from_mb
    : public boost::iterator_adaptor<
        wchar_from_mb<Base>,
        Base,
        wchar_t,
        single_pass_traversal_tag,
        wchar_t
    >
{
    friend class boost::iterator_core_access;
    typedef typename boost::iterator_adaptor<
        wchar_from_mb<Base>,
        Base,
        wchar_t,
        single_pass_traversal_tag,
        wchar_t
    > super_t;

    typedef wchar_from_mb<Base> this_t;

    void drain();

    wchar_t dereference() const {
        if(m_output.m_next == m_output.m_next_available)
            return static_cast<wchar_t>(0);
        return * m_output.m_next;
    }

    void increment(){
        if(m_output.m_next == m_output.m_next_available)
            return;
        if(++m_output.m_next == m_output.m_next_available){
            if(m_input.m_done)
                return;
            drain();
        }
    }

    bool equal(this_t const & rhs) const {
        return dereference() == rhs.dereference();
    }

    boost::archive::detail::utf8_codecvt_facet m_codecvt_facet;
    std::mbstate_t m_mbs;

    template<typename T>
    struct sliding_buffer {
        boost::array<T, 32> m_buffer;
        typename boost::array<T, 32>::const_iterator m_next_available;
        typename boost::array<T, 32>::iterator m_next;
        bool m_done;
        // default ctor
        sliding_buffer() :
            m_next_available(m_buffer.begin()),
            m_next(m_buffer.begin()),
            m_done(false)
        {}
        // copy ctor
        sliding_buffer(const sliding_buffer & rhs) :
            m_next_available(
                std::copy(
                    rhs.m_buffer.begin(),
                    rhs.m_next_available,
                    m_buffer.begin()
                )
            ),
            m_next(
                m_buffer.begin() + (rhs.m_next - rhs.m_buffer.begin())
            ),
            m_done(rhs.m_done)
        {}
    };

    sliding_buffer<typename iterator_value<Base>::type> m_input;
    sliding_buffer<typename iterator_value<this_t>::type> m_output;

public:
    // make composible buy using templated constructor
    template<class T>
    wchar_from_mb(T start) :
        super_t(Base(static_cast< T >(start))),
        m_mbs(std::mbstate_t())
    {
        BOOST_ASSERT(std::mbsinit(&m_mbs));
        drain();
    }
    // default constructor used as an end iterator
    wchar_from_mb(){}

    // copy ctor
    wchar_from_mb(const wchar_from_mb & rhs) :
        super_t(rhs.base_reference()),
        m_mbs(rhs.m_mbs),
        m_input(rhs.m_input),
        m_output(rhs.m_output)
    {}
};

template<class Base>
void wchar_from_mb<Base>::drain(){
    BOOST_ASSERT(! m_input.m_done);
    for(;;){
        typename boost::iterators::iterator_reference<Base>::type c = *(this->base_reference());
        // a null character in a multibyte stream is takes as end of string
        if(0 == c){
            m_input.m_done = true;
            break;
        }
        ++(this->base_reference());
        * const_cast<typename iterator_value<Base>::type *>(
            (m_input.m_next_available++)
        ) = c;
        // if input buffer is full - we're done for now
        if(m_input.m_buffer.end() == m_input.m_next_available)
            break;
    }
    const typename boost::iterators::iterator_value<Base>::type * input_new_start;
    typename iterator_value<this_t>::type * next_available;

    BOOST_ATTRIBUTE_UNUSED // redundant with ignore_unused below but clarifies intention
    std::codecvt_base::result r = m_codecvt_facet.in(
        m_mbs,
        m_input.m_buffer.begin(),
        m_input.m_next_available,
        input_new_start,
        m_output.m_buffer.begin(),
        m_output.m_buffer.end(),
        next_available
    );
    BOOST_ASSERT(std::codecvt_base::ok == r);
    m_output.m_next_available = next_available;
    m_output.m_next = m_output.m_buffer.begin();

    // we're done with some of the input so shift left.
    m_input.m_next_available = std::copy(
        input_new_start,
        m_input.m_next_available,
        m_input.m_buffer.begin()
    );
    m_input.m_next = m_input.m_buffer.begin();
}

} // namespace iterators
} // namespace archive
} // namespace boost

#endif // BOOST_ARCHIVE_ITERATORS_WCHAR_FROM_MB_HPP
