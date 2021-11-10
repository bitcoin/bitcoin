//  (C) Copyright Raffi Enficiaud 2018.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines a lazy/delayed dataset store
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_DELAYED_HPP_062018GER
#define BOOST_TEST_DATA_MONOMORPHIC_DELAYED_HPP_062018GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/monomorphic/fwd.hpp>
#include <boost/test/data/index_sequence.hpp>

#include <boost/core/ref.hpp>

#include <algorithm>
#include <memory>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_HDR_TUPLE)

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************               delayed_dataset                ************** //
// ************************************************************************** //


/// Delayed dataset
///
/// This dataset holds another dataset that is instanciated on demand. It is
/// constructed with the @c data::make_delayed<dataset_t>(arg1,....) instead of the
/// @c data::make.
template <class dataset_t, class ...Args>
class delayed_dataset
{
public:
    enum { arity = dataset_t::arity };
    using iterator = decltype(std::declval<dataset_t>().begin());

    delayed_dataset(Args... args)
    : m_args(std::make_tuple(std::forward<Args>(args)...))
    {}

    // Mostly for VS2013
    delayed_dataset(delayed_dataset&& b) 
    : m_args(std::move(b.m_args))
    , m_dataset(std::move(b.m_dataset))
    {}

    boost::unit_test::data::size_t size() const {
        return this->get().size();
    }

    // iterator
    iterator begin() const {
        return this->get().begin();
    }
  
private:

  dataset_t& get() const {
      if(!m_dataset) {
          m_dataset = create(boost::unit_test::data::index_sequence_for<Args...>());
      }
      return *m_dataset;
  }

  template<std::size_t... I>
  std::unique_ptr<dataset_t>
  create(boost::unit_test::data::index_sequence<I...>) const
  {
      return std::unique_ptr<dataset_t>{new dataset_t(std::get<I>(m_args)...)};
  }

  std::tuple<typename std::decay<Args>::type...> m_args;
  mutable std::unique_ptr<dataset_t> m_dataset;
};

//____________________________________________________________________________//

//! A lazy/delayed dataset is a dataset.
template <class dataset_t, class ...Args>
struct is_dataset< delayed_dataset<dataset_t, Args...> > : boost::mpl::true_ {};

//____________________________________________________________________________//

} // namespace monomorphic


//! Delayed dataset instanciation
template<class dataset_t, class ...Args>
inline typename std::enable_if<
  monomorphic::is_dataset< dataset_t >::value,
  monomorphic::delayed_dataset<dataset_t, Args...>
>::type
make_delayed(Args... args)
{
    return monomorphic::delayed_dataset<dataset_t, Args...>( std::forward<Args>(args)... );
}


} // namespace data
} // namespace unit_test
} // namespace boost

#endif

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_DELAYED_HPP_062018GER
