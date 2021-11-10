#ifndef BOOST_ARCHIVE_ITERATORS_REMOVE_WHITESPACE_HPP
#define BOOST_ARCHIVE_ITERATORS_REMOVE_WHITESPACE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// remove_whitespace.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/assert.hpp>

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_traits.hpp>

// here is the default standard implementation of the functor used
// by the filter iterator to remove spaces.  Unfortunately usage
// of this implementation in combination with spirit trips a bug
// VC 6.5.  The only way I can find to work around it is to
// implement a special non-standard version for this platform

#ifndef BOOST_NO_CWCTYPE
#include <cwctype> // iswspace
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ using ::iswspace; }
#endif
#endif

#include <cctype> // isspace
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ using ::isspace; }
#endif

#if defined(__STD_RWCOMPILER_H__) || defined(_RWSTD_VER)
// this is required for the RW STL on Linux and Tru64.
#undef isspace
#undef iswspace
#endif

namespace { // anonymous

template<class CharType>
struct remove_whitespace_predicate;

template<>
struct remove_whitespace_predicate<char>
{
    bool operator()(unsigned char t){
        return ! std::isspace(t);
    }
};

#ifndef BOOST_NO_CWCHAR
template<>
struct remove_whitespace_predicate<wchar_t>
{
    bool operator()(wchar_t t){
        return ! std::iswspace(t);
    }
};
#endif

} // namespace anonymous

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// convert base64 file data (including whitespace and padding) to binary

namespace boost {
namespace archive {
namespace iterators {

// custom version of filter iterator which doesn't look ahead further than
// necessary

template<class Predicate, class Base>
class filter_iterator
    : public boost::iterator_adaptor<
        filter_iterator<Predicate, Base>,
        Base,
        use_default,
        single_pass_traversal_tag
    >
{
    friend class boost::iterator_core_access;
    typedef typename boost::iterator_adaptor<
        filter_iterator<Predicate, Base>,
        Base,
        use_default,
        single_pass_traversal_tag
    > super_t;
    typedef filter_iterator<Predicate, Base> this_t;
    typedef typename super_t::reference reference_type;

    reference_type dereference_impl(){
        if(! m_full){
            while(! m_predicate(* this->base_reference()))
                ++(this->base_reference());
            m_full = true;
        }
        return * this->base_reference();
    }

    reference_type dereference() const {
        return const_cast<this_t *>(this)->dereference_impl();
    }

    Predicate m_predicate;
    bool m_full;
public:
    // note: this function is public only because comeau compiler complained
    // I don't know if this is because the compiler is wrong or what
    void increment(){
        m_full = false;
        ++(this->base_reference());
    }
    filter_iterator(Base start) :
        super_t(start),
        m_full(false)
    {}
    filter_iterator(){}
};

template<class Base>
class remove_whitespace :
    public filter_iterator<
        remove_whitespace_predicate<
            typename boost::iterator_value<Base>::type
            //typename Base::value_type
        >,
        Base
    >
{
    friend class boost::iterator_core_access;
    typedef filter_iterator<
        remove_whitespace_predicate<
            typename boost::iterator_value<Base>::type
            //typename Base::value_type
        >,
        Base
    > super_t;
public:
//    remove_whitespace(){} // why is this needed?
    // make composible buy using templated constructor
    template<class T>
    remove_whitespace(T start) :
        super_t(Base(static_cast< T >(start)))
    {}
    // intel 7.1 doesn't like default copy constructor
    remove_whitespace(const remove_whitespace & rhs) :
        super_t(rhs.base_reference())
    {}
};

} // namespace iterators
} // namespace archive
} // namespace boost

#endif // BOOST_ARCHIVE_ITERATORS_REMOVE_WHITESPACE_HPP
