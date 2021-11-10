//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_OSTREAM_ITERATOR_MAY_26_2007_1016PM)
#define BOOST_SPIRIT_KARMA_OSTREAM_ITERATOR_MAY_26_2007_1016PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <iterator>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma 
{
    ///////////////////////////////////////////////////////////////////////////
    //  We need our own implementation of an ostream_iterator just to be able
    //  to access the wrapped ostream, which is necessary for the 
    //  stream_generator, where we must generate the output using the original
    //  ostream to retain possibly registered facets.
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T, typename Elem = char
      , typename Traits = std::char_traits<Elem> >
    class ostream_iterator 
    {
    public:
        typedef std::output_iterator_tag iterator_category;
        typedef void value_type;
        typedef void difference_type;
        typedef void pointer;
        typedef void reference;
        typedef Elem char_type;
        typedef Traits traits_type;
        typedef std::basic_ostream<Elem, Traits> ostream_type;
        typedef ostream_iterator<T, Elem, Traits> self_type;

        ostream_iterator(ostream_type& os_, Elem const* delim_ = 0)
          : os(&os_), delim(delim_) {}

        self_type& operator= (T const& val)
        {
            *os << val;
            if (0 != delim)
                *os << delim;
            return *this;
        }

        self_type& operator*() { return *this; }
        self_type& operator++() { return *this; }
        self_type operator++(int) { return *this; }

        // expose underlying stream
        ostream_type& get_ostream() { return *os; }
        ostream_type const& get_ostream() const { return *os; }

        // expose good bit of underlying stream object
        bool good() const { return get_ostream().good(); }

    protected:
        ostream_type *os;
        Elem const* delim;
    };

}}}

#endif
