// Copyright Eric Niebler 2005.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef STL_ITERATOR_EAN20051028_HPP
# define STL_ITERATOR_EAN20051028_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/object/stl_iterator_core.hpp>

# include <boost/iterator/iterator_facade.hpp>

namespace boost { namespace python
{ 

// An STL input iterator over a python sequence
template<typename ValueT>
struct stl_input_iterator
  : boost::iterator_facade<
        stl_input_iterator<ValueT>
      , ValueT
      , std::input_iterator_tag
      , ValueT
    >
{
    stl_input_iterator()
      : impl_()
    {
    }

    // ob is the python sequence
    stl_input_iterator(boost::python::object const &ob)
      : impl_(ob)
    {
    }

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        this->impl_.increment();
    }

    ValueT dereference() const
    {
        return extract<ValueT>(this->impl_.current().get())();
    }

    bool equal(stl_input_iterator<ValueT> const &that) const
    {
        return this->impl_.equal(that.impl_);
    }

    objects::stl_input_iterator_impl impl_;
};

}} // namespace boost::python

#endif // STL_ITERATOR_EAN20051028_HPP
