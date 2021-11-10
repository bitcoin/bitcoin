//  (C) Copyright Joel de Guzman 2003.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef INDEXING_SUITE_JDG20036_HPP
# define INDEXING_SUITE_JDG20036_HPP

# include <boost/python/class.hpp>
# include <boost/python/def_visitor.hpp>
# include <boost/python/register_ptr_to_python.hpp>
# include <boost/python/suite/indexing/detail/indexing_suite_detail.hpp>
# include <boost/python/return_internal_reference.hpp>
# include <boost/python/iterator.hpp>
# include <boost/mpl/or.hpp>
# include <boost/mpl/not.hpp>
# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python {

    // indexing_suite class. This class is the facade class for
    // the management of C++ containers intended to be integrated
    // to Python. The objective is make a C++ container look and
    // feel and behave exactly as we'd expect a Python container.
    // By default indexed elements are returned by proxy. This can be
    // disabled by supplying *true* in the NoProxy template parameter.
    //
    // Derived classes provide the hooks needed by the indexing_suite
    // to do its job:
    //
    //      static data_type&
    //      get_item(Container& container, index_type i);
    //
    //      static object
    //      get_slice(Container& container, index_type from, index_type to);
    //
    //      static void
    //      set_item(Container& container, index_type i, data_type const& v);
    //
    //      static void
    //      set_slice(
    //         Container& container, index_type from,
    //         index_type to, data_type const& v
    //      );
    //
    //      template <class Iter>
    //      static void
    //      set_slice(Container& container, index_type from,
    //          index_type to, Iter first, Iter last
    //      );
    //
    //      static void
    //      delete_item(Container& container, index_type i);
    //
    //      static void
    //      delete_slice(Container& container, index_type from, index_type to);
    //
    //      static size_t
    //      size(Container& container);
    //
    //      template <class T>
    //      static bool
    //      contains(Container& container, T const& val);
    //
    //      static index_type
    //      convert_index(Container& container, PyObject* i);
    //
    //      static index_type
    //      adjust_index(index_type current, index_type from,
    //          index_type to, size_type len
    //      );
    //
    // Most of these policies are self explanatory. convert_index and
    // adjust_index, however, deserves some explanation.
    //
    // convert_index converts an Python index into a C++ index that the
    // container can handle. For instance, negative indexes in Python, by
    // convention, indexes from the right (e.g. C[-1] indexes the rightmost
    // element in C). convert_index should handle the necessary conversion
    // for the C++ container (e.g. convert -1 to C.size()-1). convert_index
    // should also be able to convert the type of the index (A dynamic Python
    // type) to the actual type that the C++ container expects.
    //
    // When a container expands or contracts, held indexes to its elements
    // must be adjusted to follow the movement of data. For instance, if
    // we erase 3 elements, starting from index 0 from a 5 element vector,
    // what used to be at index 4 will now be at index 1:
    //
    //      [a][b][c][d][e] ---> [d][e]
    //                   ^           ^
    //                   4           1
    //
    // adjust_index takes care of the adjustment. Given a current index,
    // the function should return the adjusted index when data in the
    // container at index from..to is replaced by *len* elements.
    //

    template <
          class Container
        , class DerivedPolicies
        , bool NoProxy = false
        , bool NoSlice = false
        , class Data = typename Container::value_type
        , class Index = typename Container::size_type
        , class Key = typename Container::value_type
    >
    class indexing_suite
        : public def_visitor<
            indexing_suite<
              Container
            , DerivedPolicies
            , NoProxy
            , NoSlice
            , Data
            , Index
            , Key
        > >
    {
    private:

        typedef mpl::or_<
            mpl::bool_<NoProxy>
          , mpl::not_<is_class<Data> >
          , typename mpl::or_<
                detail::is_same<Data, std::string>
              , detail::is_same<Data, std::complex<float> >
              , detail::is_same<Data, std::complex<double> >
              , detail::is_same<Data, std::complex<long double> > >::type>
        no_proxy;

        typedef detail::container_element<Container, Index, DerivedPolicies>
            container_element_t;

        typedef return_internal_reference<> return_policy;

        typedef typename mpl::if_<
            no_proxy
          , iterator<Container>
          , iterator<Container, return_policy> >::type
        def_iterator;

        typedef typename mpl::if_<
            no_proxy
          , detail::no_proxy_helper<
                Container
              , DerivedPolicies
              , container_element_t
              , Index>
          , detail::proxy_helper<
                Container
              , DerivedPolicies
              , container_element_t
              , Index> >::type
        proxy_handler;

        typedef typename mpl::if_<
            mpl::bool_<NoSlice>
          , detail::no_slice_helper<
                Container
              , DerivedPolicies
              , proxy_handler
              , Data
              , Index>
          , detail::slice_helper<
                Container
              , DerivedPolicies
              , proxy_handler
              , Data
              , Index> >::type
        slice_handler;

    public:

        template <class Class>
        void visit(Class& cl) const
        {
            // Hook into the class_ generic visitation .def function
            proxy_handler::register_container_element();

            cl
                .def("__len__", base_size)
                .def("__setitem__", &base_set_item)
                .def("__delitem__", &base_delete_item)
                .def("__getitem__", &base_get_item)
                .def("__contains__", &base_contains)
                .def("__iter__", def_iterator())
            ;

            DerivedPolicies::extension_def(cl);
        }

        template <class Class>
        static void
        extension_def(Class& cl)
        {
            // default.
            // no more extensions
        }

    private:

        static object
        base_get_item(back_reference<Container&> container, PyObject* i)
        {
            if (PySlice_Check(i))
                return slice_handler::base_get_slice(
                    container.get(), static_cast<PySliceObject*>(static_cast<void*>(i)));

            return proxy_handler::base_get_item_(container, i);
        }

        static void
        base_set_item(Container& container, PyObject* i, PyObject* v)
        {
            if (PySlice_Check(i))
            {
                 slice_handler::base_set_slice(container,
                     static_cast<PySliceObject*>(static_cast<void*>(i)), v);
            }
            else
            {
                extract<Data&> elem(v);
                // try if elem is an exact Data
                if (elem.check())
                {
                    DerivedPolicies::
                        set_item(container,
                            DerivedPolicies::
                                convert_index(container, i), elem());
                }
                else
                {
                    //  try to convert elem to Data
                    extract<Data> elem(v);
                    if (elem.check())
                    {
                        DerivedPolicies::
                            set_item(container,
                                DerivedPolicies::
                                    convert_index(container, i), elem());
                    }
                    else
                    {
                        PyErr_SetString(PyExc_TypeError, "Invalid assignment");
                        throw_error_already_set();
                    }
                }
            }
        }

        static void
        base_delete_item(Container& container, PyObject* i)
        {
            if (PySlice_Check(i))
            {
                slice_handler::base_delete_slice(
                    container, static_cast<PySliceObject*>(static_cast<void*>(i)));
                return;
            }

            Index index = DerivedPolicies::convert_index(container, i);
            proxy_handler::base_erase_index(container, index, mpl::bool_<NoSlice>());
            DerivedPolicies::delete_item(container, index);
        }

        static size_t
        base_size(Container& container)
        {
            return DerivedPolicies::size(container);
        }

        static bool
        base_contains(Container& container, PyObject* key)
        {
            extract<Key const&> x(key);
            //  try if key is an exact Key type
            if (x.check())
            {
                return DerivedPolicies::contains(container, x());
            }
            else
            {
                //  try to convert key to Key type
                extract<Key> x(key);
                if (x.check())
                    return DerivedPolicies::contains(container, x());
                else
                    return false;
            }
        }
    };

}} // namespace boost::python

#endif // INDEXING_SUITE_JDG20036_HPP
