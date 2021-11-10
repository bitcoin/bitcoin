#ifndef BOOST_SERIALIZATION_COLLECTION_SIZE_TYPE_HPP
#define BOOST_SERIALIZATION_COLLECTION_SIZE_TYPE_HPP

// (C) Copyright 2005  Matthias Troyer
// (C) Copyright 2020  Robert Ramey

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef> // size_t
#include <boost/assert.hpp>
#include <boost/operators.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>

namespace boost {
namespace serialization {

template<typename T> // T is the basic type holding the integer
struct cardinal_number : private
    boost::totally_ordered<cardinal_number<T> >,
    boost::additive<cardinal_number<T> >,
    boost::unit_steppable<cardinal_number<T> >
{
private:
    template<typename CharType, typename Traits>
    friend std::basic_ostream<CharType, Traits> & operator<<(
        const std::basic_ostream<CharType, Traits> & bos,
        const cardinal_number & rhs
    );
    template<typename CharType, typename Traits>
    friend std::basic_istream<CharType, Traits> & operator>>(
        std::basic_istream<CharType, Traits> & bis,
        const cardinal_number & rhs
    );
public:
    T m_t;
    cardinal_number(T t = 0) :
        m_t(t)
    {}
    cardinal_number(unsigned int t) :
        m_t(t)
    {}
    cardinal_number(int t) :
        m_t(t)
    {
        BOOST_ASSERT(t >= 0);
    }
    operator const T (){
        return m_t;
    }
    // assignment operator
    cardinal_number & operator=(const cardinal_number & rhs){
        m_t = rhs.m_t;
        return *this;
    }
    // basic operations upon which others depend
    // totally ordered / less_than_comparable
    bool operator<(const cardinal_number & rhs) const {
        return m_t < rhs.m_t;
    }
    bool operator==(const cardinal_number & rhs) const {
        return m_t == rhs.m_t;
    }
    // additive
    cardinal_number & operator+=(const cardinal_number & rhs){
        m_t += rhs.m_t;
        return *this;
    }
    // subtractive
    cardinal_number & operator-=(const cardinal_number & rhs){
        BOOST_ASSERT(m_t >= rhs.m_t);
        m_t -= rhs.m_t;
        return *this;
    }
    // increment
    cardinal_number operator++(){
        ++m_t;
        return *this;
     }
     // decrement
    cardinal_number operator--(){
        BOOST_ASSERT(m_t > T(0));
        --m_t;
        return *this;
     }
};

typedef cardinal_number<std::size_t> collection_size_type;

} } // end namespace boost::serialization

#include <ostream>
#include <istream>

namespace std {

template<typename CharType, typename Traits>
basic_ostream<CharType, Traits> & operator<<(
    std::basic_ostream<CharType, Traits> & bos,
    const boost::serialization::collection_size_type & rhs
){
    bos << rhs.m_t;
    return bos;
}

template<typename CharType, typename Traits>
basic_istream<CharType, Traits> & operator>>(
    std::basic_istream<CharType, Traits> & bis,
    boost::serialization::collection_size_type & rhs
){
    bis >> rhs.m_t;
    return bis;
}

} // std

BOOST_CLASS_IMPLEMENTATION(                     \
    boost::serialization::collection_size_type, \
    primitive_type                              \
)
BOOST_IS_BITWISE_SERIALIZABLE(boost::serialization::collection_size_type)

#endif //BOOST_SERIALIZATION_COLLECTION_SIZE_TYPE_HPP
