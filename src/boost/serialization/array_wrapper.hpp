#ifndef BOOST_SERIALIZATION_ARRAY_WRAPPER_HPP
#define BOOST_SERIALIZATION_ARRAY_WRAPPER_HPP

// (C) Copyright 2005 Matthias Troyer and Dave Abrahams
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp> // msvc 6.0 needs this for warning suppression

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/wrapper.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/array_optimization.hpp>
#include <boost/mpl/always.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/bool_fwd.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace boost { namespace serialization {

template<class T>
class array_wrapper :
    public wrapper_traits<const array_wrapper< T > >
{
private:
    array_wrapper & operator=(const array_wrapper & rhs);
    // note: I would like to make the copy constructor private but this breaks
    // make_array.  So I make make_array a friend
    template<class Tx, class S>
    friend const boost::serialization::array_wrapper<Tx> make_array(Tx * t, S s);
public:

    array_wrapper(const array_wrapper & rhs) :
        m_t(rhs.m_t),
        m_element_count(rhs.m_element_count)
    {}
public:
    array_wrapper(T * t, std::size_t s) :
        m_t(t),
        m_element_count(s)
    {}

    // default implementation
    template<class Archive>
    void serialize_optimized(Archive &ar, const unsigned int, mpl::false_ ) const
    {
      // default implemention does the loop
      std::size_t c = count();
      T * t = address();
      while(0 < c--)
            ar & boost::serialization::make_nvp("item", *t++);
    }

    // optimized implementation
    template<class Archive>
    void serialize_optimized(Archive &ar, const unsigned int version, mpl::true_ )
    {
      boost::serialization::split_member(ar, *this, version);
    }

    // default implementation
    template<class Archive>
    void save(Archive &ar, const unsigned int version) const
    {
      ar.save_array(*this,version);
    }

    // default implementation
    template<class Archive>
    void load(Archive &ar, const unsigned int version)
    {
      ar.load_array(*this,version);
    }

    // default implementation
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
      typedef typename
          boost::serialization::use_array_optimization<Archive>::template apply<
                    typename remove_const< T >::type
                >::type use_optimized;
      serialize_optimized(ar,version,use_optimized());
    }

    T * address() const
    {
      return m_t;
    }

    std::size_t count() const
    {
      return m_element_count;
    }

private:
    T * const m_t;
    const std::size_t m_element_count;
};

template<class T, class S>
inline
const array_wrapper< T > make_array(T* t, S s){
    const array_wrapper< T > a(t, s);
    return a;
}

} } // end namespace boost::serialization


#endif //BOOST_SERIALIZATION_ARRAY_WRAPPER_HPP
