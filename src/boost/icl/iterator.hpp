/*-----------------------------------------------------------------------------+
Copyright (c) 2009-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_ITERATOR_HPP_JOFA_091003
#define BOOST_ICL_ITERATOR_HPP_JOFA_091003

#include <iterator>
#include <boost/config/warning_disable.hpp>

namespace boost{namespace icl
{

/** \brief Performes an addition using a container's memberfunction add, when operator= is called. */
template<class ContainerT> class add_iterator
{
public:
    /// The container's type.
    typedef ContainerT container_type;
    typedef std::output_iterator_tag iterator_category; 
    typedef void value_type;
    typedef void difference_type;
    typedef void pointer;
    typedef void reference;

    /** An add_iterator is constructed with a container and a position 
        that has to be maintained. */
    add_iterator(ContainerT& cont, typename ContainerT::iterator iter)
    : _cont(&cont), _iter(iter) {}

    /** This assignment operator adds the \c value before the current position.
        It maintains it's position by incrementing after addition.    */
    add_iterator& operator=(typename ContainerT::const_reference value)
    {
        _iter = icl::add(*_cont, _iter, value);
        if(_iter != _cont->end())
            ++_iter;
        return *this;
    }

    add_iterator& operator*()    { return *this; }
    add_iterator& operator++()   { return *this; }
    add_iterator& operator++(int){ return *this; }

private:
    ContainerT*                   _cont;
    typename ContainerT::iterator _iter;
};


/** Function adder creates and initializes an add_iterator */
template<class ContainerT, typename IteratorT>
inline add_iterator<ContainerT> adder(ContainerT& cont, IteratorT iter_)
{
    return add_iterator<ContainerT>(cont, typename ContainerT::iterator(iter_));
}

/** \brief Performes an insertion using a container's memberfunction add, when operator= is called. */
template<class ContainerT> class insert_iterator
{
public:
    /// The container's type.
    typedef ContainerT container_type;
    typedef std::output_iterator_tag iterator_category; 
    typedef void value_type;
    typedef void difference_type;
    typedef void pointer;
    typedef void reference;

    /** An insert_iterator is constructed with a container and a position 
        that has to be maintained. */
    insert_iterator(ContainerT& cont, typename ContainerT::iterator iter)
    : _cont(&cont), _iter(iter) {}

    /** This assignment operator adds the \c value before the current position.
        It maintains it's position by incrementing after addition.    */
    insert_iterator& operator=(typename ContainerT::const_reference value)
    {
        _iter = _cont->insert(_iter, value);
        if(_iter != _cont->end())
            ++_iter;
        return *this;
    }

    insert_iterator& operator*()    { return *this; }
    insert_iterator& operator++()   { return *this; }
    insert_iterator& operator++(int){ return *this; }

private:
    ContainerT*                   _cont;
    typename ContainerT::iterator _iter;
};


/** Function inserter creates and initializes an insert_iterator */
template<class ContainerT, typename IteratorT>
inline insert_iterator<ContainerT> inserter(ContainerT& cont, IteratorT iter_)
{
    return insert_iterator<ContainerT>(cont, typename ContainerT::iterator(iter_));
}

}} // namespace icl boost

#endif // BOOST_ICL_ITERATOR_HPP_JOFA_091003


