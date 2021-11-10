// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef LIST_DWA2002627_HPP
# define LIST_DWA2002627_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/object.hpp>
# include <boost/python/converter/pytype_object_mgr_traits.hpp>
# include <boost/python/ssize_t.hpp>

namespace boost { namespace python { 

namespace detail
{
  struct BOOST_PYTHON_DECL list_base : object
  {
      void append(object_cref); // append object to end

      ssize_t count(object_cref value) const; // return number of occurrences of value

      void extend(object_cref sequence); // extend list by appending sequence elements
    
      long index(object_cref value) const; // return index of first occurrence of value

      void insert(ssize_t index, object_cref); // insert object before index
      void insert(object const& index, object_cref);

      object pop(); // remove and return item at index (default last)
      object pop(ssize_t index);
      object pop(object const& index);

      void remove(object_cref value); // remove first occurrence of value
    
      void reverse(); // reverse *IN PLACE*

      void sort(); //  sort *IN PLACE*; if given, cmpfunc(x, y) -> -1, 0, 1
#if PY_VERSION_HEX >= 0x03000000
      void sort(args_proxy const &args, 
                 kwds_proxy const &kwds);
#else
      void sort(object_cref cmpfunc);
#endif

   protected:
      list_base(); // new list
      explicit list_base(object_cref sequence); // new list initialized from sequence's items

      BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(list_base, object)
   private:    
      static detail::new_non_null_reference call(object const&);
  };
}

class list : public detail::list_base
{
    typedef detail::list_base base;
 public:
    list() {} // new list

    template <class T>
    explicit list(T const& sequence)
        : base(object(sequence))
    {
    }

    template <class T>
    void append(T const& x)
    {
        base::append(object(x));
    }

    template <class T>
    ssize_t count(T const& value) const
    {
        return base::count(object(value));
    }
    
    template <class T>
    void extend(T const& x)
    {
        base::extend(object(x));
    }

    template <class T>
    long index(T const& x) const
    {
        return base::index(object(x));
    }
    
    template <class T>
    void insert(ssize_t index, T const& x) // insert object before index
    {
        base::insert(index, object(x));
    }
    
    template <class T>
    void insert(object const& index, T const& x) // insert object before index
    {
        base::insert(index, object(x));
    }

    object pop() { return base::pop(); }
    object pop(ssize_t index) { return base::pop(index); }
    
    template <class T>
    object pop(T const& index)
    {
        return base::pop(object(index));
    }

    template <class T>
    void remove(T const& value)
    {
        base::remove(object(value));
    }

#if PY_VERSION_HEX <= 0x03000000
    void sort() { base::sort(); }

    template <class T>
    void sort(T const& value)
    {
        base::sort(object(value));
    }
#endif
    
 public: // implementation detail -- for internal use only
    BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(list, base)
};

//
// Converter Specializations
//
namespace converter
{
  template <>
  struct object_manager_traits<list>
      : pytype_object_manager_traits<&PyList_Type,list>
  {
  };
}

}} // namespace boost::python

#endif // LIST_DWA2002627_HPP
