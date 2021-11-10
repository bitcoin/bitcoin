#ifndef  BOOST_SERIALIZATION_DETAIL_STACK_CONSTRUCTOR_HPP
#define BOOST_SERIALIZATION_DETAIL_STACK_CONSTRUCTOR_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// stack_constructor.hpp: serialization for loading stl collections

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/aligned_storage.hpp>
#include <boost/serialization/serialization.hpp>

namespace boost{
namespace serialization {
namespace detail {

// reserve space on stack for an object of type T without actually
// construction such an object
template<typename T >
struct stack_allocate
{
    T * address() {
        return static_cast<T*>(storage_.address());
    }
    T & reference() {
        return * address();
    }
private:
    typedef typename boost::aligned_storage<
        sizeof(T),
        boost::alignment_of<T>::value
    > type;
    type storage_;
};

// construct element on the stack
template<class Archive, class T>
struct stack_construct : public stack_allocate<T>
{
    stack_construct(Archive & ar, const unsigned int version){
        // note borland emits a no-op without the explicit namespace
        boost::serialization::load_construct_data_adl(
            ar,
            this->address(),
            version
        );
    }
    ~stack_construct(){
        this->address()->~T(); // undo load_construct_data above
    }
};

} // detail
} // serializaition
} // boost

#endif //  BOOST_SERIALIZATION_DETAIL_STACH_CONSTRUCTOR_HPP
