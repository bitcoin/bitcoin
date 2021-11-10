// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef STR_20020703_HPP
#define STR_20020703_HPP

# include <boost/python/detail/prefix.hpp>

#include <boost/python/object.hpp>
#include <boost/python/list.hpp>
#include <boost/python/converter/pytype_object_mgr_traits.hpp>

// disable defines in <cctype> provided by some system libraries
#undef isspace
#undef islower
#undef isalpha
#undef isdigit
#undef isalnum
#undef isupper

namespace boost { namespace python {

class str;

namespace detail
{
  struct BOOST_PYTHON_DECL str_base : object
  {
      str capitalize() const;

      str center(object_cref width) const;

      long count(object_cref sub) const;

      long count(object_cref sub, object_cref start) const;
    
      long count(object_cref sub, object_cref start, object_cref end) const;

#if PY_VERSION_HEX < 0x03000000
      object decode() const;
      object decode(object_cref encoding) const;

      object decode(object_cref encoding, object_cref errors) const;
#endif

      object encode() const;
      object encode(object_cref encoding) const;
      object encode(object_cref encoding, object_cref errors) const;

      bool endswith(object_cref suffix) const;
    
      bool endswith(object_cref suffix, object_cref start) const;
      bool endswith(object_cref suffix, object_cref start, object_cref end) const;
    
      str expandtabs() const;
      str expandtabs(object_cref tabsize) const;

      long find(object_cref sub) const;
      long find(object_cref sub, object_cref start) const;

      long find(object_cref sub, object_cref start, object_cref end) const;

      long index(object_cref sub) const;

      long index(object_cref sub, object_cref start) const;
      long index(object_cref sub, object_cref start, object_cref end) const;

      bool isalnum() const;
      bool isalpha() const;
      bool isdigit() const;
      bool islower() const;
      bool isspace() const;
      bool istitle() const;
      bool isupper() const;
    
      str join(object_cref sequence) const;

      str ljust(object_cref width) const;
      str lower() const;
      str lstrip() const;

      str replace(object_cref old, object_cref new_) const;
      str replace(object_cref old, object_cref new_, object_cref maxsplit) const;
      long rfind(object_cref sub) const;

      long rfind(object_cref sub, object_cref start) const;

      long rfind(object_cref sub, object_cref start, object_cref end) const;
      long rindex(object_cref sub) const;
      long rindex(object_cref sub, object_cref start) const;


      long rindex(object_cref sub, object_cref start, object_cref end) const;

      str rjust(object_cref width) const;
    
      str rstrip() const;
    
      list split() const; 
      list split(object_cref sep) const;
   
      list split(object_cref sep, object_cref maxsplit) const; 
    

      list splitlines() const;
      list splitlines(object_cref keepends) const;

      bool startswith(object_cref prefix) const;


      bool startswith(object_cref prefix, object_cref start) const;
      bool startswith(object_cref prefix, object_cref start, object_cref end) const;

      str strip() const;
      str swapcase() const;
      str title() const;
    
      str translate(object_cref table) const;

      str translate(object_cref table, object_cref deletechars) const;

    
      str upper() const;

   protected:
      str_base(); // new str
    
      str_base(const char* s); // new str

      str_base(char const* start, char const* finish);
      
      str_base(char const* start, std::size_t length);
      
      explicit str_base(object_cref other);

      BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(str_base, object)
   private:
      static new_reference call(object const&);
  };
}


class str : public detail::str_base
{
    typedef detail::str_base base;
 public:
    str() {} // new str
    
    str(const char* s) : base(s) {} // new str
    
    str(char const* start, char const* finish) // new str
      : base(start, finish)
    {}
    
    str(char const* start, std::size_t length) // new str
      : base(start, length)
    {}
    
    template <class T>
    explicit str(T const& other)
        : base(object(other))
    {
    }

    template <class T>
    str center(T const& width) const
    {
        return base::center(object(width));
    }

    template<class T>
    long count(T const& sub) const
    {
        return base::count(object(sub));
    }

    template<class T1, class T2>
    long count(T1 const& sub,T2 const& start) const
    {
        return base::count(object(sub), object(start));
    }

    template<class T1, class T2, class T3>
    long count(T1 const& sub,T2 const& start, T3 const& end) const
    {
        return base::count(object(sub), object(start), object(end));
    }

#if PY_VERSION_HEX < 0x03000000
    object decode() const { return base::decode(); }
    
    template<class T>
    object decode(T const& encoding) const
    {
        return base::decode(object(encoding));
    }

    template<class T1, class T2>
    object decode(T1 const& encoding, T2 const& errors) const
    {
        return base::decode(object(encoding),object(errors));
    }
#endif

    object encode() const { return base::encode(); }

    template <class T>
    object encode(T const& encoding) const
    {
        return base::encode(object(encoding));
    }

    template <class T1, class T2>
    object encode(T1 const& encoding, T2 const& errors) const
    {
        return base::encode(object(encoding),object(errors));
    }

    template <class T>
    bool endswith(T const& suffix) const
    {
        return base::endswith(object(suffix));
    }

    template <class T1, class T2>
    bool endswith(T1 const& suffix, T2 const& start) const
    {
        return base::endswith(object(suffix), object(start));
    }

    template <class T1, class T2, class T3>
    bool endswith(T1 const& suffix, T2 const& start, T3 const& end) const
    {
        return base::endswith(object(suffix), object(start), object(end));
    }
    
    str expandtabs() const { return base::expandtabs(); }

    template <class T>
    str expandtabs(T const& tabsize) const
    {
        return base::expandtabs(object(tabsize));
    }
    
    template <class T>
    long find(T const& sub) const
    {
        return base::find(object(sub));
    }

    template <class T1, class T2>
    long find(T1 const& sub, T2 const& start) const
    {
        return base::find(object(sub), object(start));
    }

    template <class T1, class T2, class T3>
    long find(T1 const& sub, T2 const& start, T3 const& end) const
    {
        return base::find(object(sub), object(start), object(end));
    }
    
    template <class T>
    long index(T const& sub) const
    {
        return base::index(object(sub));
    }
    
    template <class T1, class T2>
    long index(T1 const& sub, T2 const& start) const
    {
        return base::index(object(sub), object(start));
    }

    template <class T1, class T2, class T3>
    long index(T1 const& sub, T2 const& start, T3 const& end) const
    {
        return base::index(object(sub), object(start), object(end));
    }

    template <class T>
    str join(T const& sequence) const
    {
        return base::join(object(sequence));
    }
    
    template <class T>
    str ljust(T const& width) const
    {
        return base::ljust(object(width));
    }

    template <class T1, class T2>
    str replace(T1 const& old, T2 const& new_) const 
    {
        return base::replace(object(old),object(new_));
    }

    template <class T1, class T2, class T3>
    str replace(T1 const& old, T2 const& new_, T3 const& maxsplit) const 
    {
        return base::replace(object(old),object(new_), object(maxsplit));
    }
    
    template <class T>
    long rfind(T const& sub) const
    {
        return base::rfind(object(sub));
    }

    template <class T1, class T2>
    long rfind(T1 const& sub, T2 const& start) const
    {
        return base::rfind(object(sub), object(start));
    }
    
    template <class T1, class T2, class T3>
    long rfind(T1 const& sub, T2 const& start, T3 const& end) const
    {
        return base::rfind(object(sub), object(start), object(end));
    }
    
    template <class T>
    long rindex(T const& sub) const
    {
        return base::rindex(object(sub));
    }

    template <class T1, class T2>
    long rindex(T1 const& sub, T2 const& start) const
    {
        return base::rindex(object(sub), object(start));
    }

    template <class T1, class T2, class T3>
    long rindex(T1 const& sub, T2 const& start, T3 const& end) const
    {
        return base::rindex(object(sub), object(start), object(end));
    }

    template <class T>
    str rjust(T const& width) const
    {
        return base::rjust(object(width));
    }
    
    list split() const { return base::split(); }
   
    template <class T>
    list split(T const& sep) const
    {
        return base::split(object(sep));
    }

    template <class T1, class T2>
    list split(T1 const& sep, T2 const& maxsplit) const
    {
        return base::split(object(sep), object(maxsplit));
    }

    list splitlines() const { return base::splitlines(); }

    template <class T>
    list splitlines(T const& keepends) const
    {
        return base::splitlines(object(keepends));
    }

    template <class T>
    bool startswith(T const& prefix) const
    {
        return base::startswith(object(prefix));
    }

    template <class T1, class T2>
    bool startswith(T1 const& prefix, T2 const& start) const
    {
        return base::startswith(object(prefix), object(start));
    }
     
    template <class T1, class T2, class T3>
    bool startswith(T1 const& prefix, T2 const& start, T3 const& end) const
    {
        return base::startswith(object(prefix), object(start), object(end));
    }

    template <class T>
    str translate(T const& table) const
    {
        return base::translate(object(table));
    }

    template <class T1, class T2>
    str translate(T1 const& table, T2 const& deletechars) const
    {
        return base::translate(object(table), object(deletechars));
    }
    
 public: // implementation detail -- for internal use only
    BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(str, base)
};

//
// Converter Specializations
//
namespace converter
{
  template <>
  struct object_manager_traits<str>
#if PY_VERSION_HEX >= 0x03000000
      : pytype_object_manager_traits<&PyUnicode_Type,str>
#else
      : pytype_object_manager_traits<&PyString_Type,str>
#endif
  {
  };
}

}}  // namespace boost::python

#endif // STR_20020703_HPP
