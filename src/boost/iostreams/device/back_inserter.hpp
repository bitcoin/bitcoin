// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_BACK_INSERTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_BACK_INSERTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/iostreams/detail/ios.hpp> // streamsize.
#include <boost/iostreams/categories.hpp>

namespace boost { namespace iostreams {

template<typename Container>
class back_insert_device {
public:
    typedef typename Container::value_type  char_type;
    typedef sink_tag                        category;
    back_insert_device(Container& cnt) : container(&cnt) { }
    std::streamsize write(const char_type* s, std::streamsize n)
    { 
        container->insert(container->end(), s, s + n); 
        return n;
    }
protected:
    Container* container;
};

template<typename Container>
back_insert_device<Container> back_inserter(Container& cnt)
{ return back_insert_device<Container>(cnt); }

} } // End namespaces iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_BACK_INSERTER_HPP_INCLUDED
