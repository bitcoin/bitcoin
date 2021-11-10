/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2010-2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_UTREE)
#define BOOST_SPIRIT_UTREE

#include <cstddef>
#include <algorithm>
#include <string>
#include <iostream>
#include <ios>
#include <sstream>
#include <typeinfo>

#include <boost/io/ios_state.hpp>
#include <boost/integer.hpp>
#include <boost/throw_exception.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/ref.hpp>
#include <boost/config.hpp>

#include <boost/spirit/home/support/utree/detail/utree_detail1.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4804)
# pragma warning(disable: 4805)
# pragma warning(disable: 4244)
#endif

namespace boost { namespace spirit
{
    //[utree_exceptions
    /*` All exceptions thrown by utree are derived from utree_exception. */
    struct BOOST_SYMBOL_VISIBLE utree_exception : std::exception {};

    /*`The `bad_type_exception` is thrown whenever somebody calls a member 
       function, which applies to certain stored utree_type's only, but this 
       precondition is violated as the `utree` instance holds some other type.
    */
    struct bad_type_exception /*: utree_exception*/;

    /*`The `empty_exception` is thrown whenever a precondition of a list
       or range utree method is violated due to the list or range being empty. 
    */
    struct empty_exception /*: utree_exception*/;
    //]

    //[utree_types
    /*`Each instance of an `utree` data structure can store exactly one of the 
       following data types at a time: 
    */
    struct utree_type
    {
        enum info
        {
            invalid_type,       // the utree has not been initialized (it's 
                                // default constructed)
            nil_type,           // nil is the sentinel (empty) utree type.
            list_type,          // A doubly linked list of utrees.
            range_type,         // A range of list::iterators. 
            reference_type,     // A reference to another utree.
            any_type,           // A pointer or reference to any C++ type. 
            function_type,      // A utree holding a stored_function<F> object,
                                // where F is an unary function object taking a 
                                // utree as it's parameter and returning a
                                // utree.

            // numeric atoms
            bool_type,          // An utree holding a boolean value
            int_type,           // An utree holding a integer (int) value
            double_type,        // An utree holding a floating point (double) value

            // text atoms (utf8)
            string_type,        // An UTF-8 string 
            string_range_type,  // A pair of iterators into an UTF-8 string
            symbol_type,        // An UTF-8 symbol name

            binary_type         // Arbitrary binary data
        };
        typedef boost::uint_t<sizeof(info)*8>::exact exact_integral_type; 
        typedef boost::uint_t<sizeof(info)*8>::fast fast_integral_type; 
    };
    //]

    // streaming operator for utree types - essential for diagnostics    
    inline std::ostream& operator<<(std::ostream& out, utree_type::info t)
    {
        boost::io::ios_all_saver saver(out);
        switch (t) {
            case utree_type::invalid_type: { out << "invalid"; break; }
            case utree_type::nil_type: { out << "nil"; break; }
            case utree_type::list_type: { out << "list"; break; }
            case utree_type::range_type: { out << "range"; break; }
            case utree_type::reference_type: { out << "reference"; break; }
            case utree_type::any_type: { out << "any"; break; }
            case utree_type::function_type: { out << "function"; break; }
            case utree_type::bool_type: { out << "bool"; break; }
            case utree_type::int_type: { out << "int"; break; }
            case utree_type::double_type: { out << "double"; break; }
            case utree_type::string_type: { out << "string"; break; }
            case utree_type::string_range_type: { out << "string_range"; break; }
            case utree_type::symbol_type: { out << "symbol"; break; }
            case utree_type::binary_type: { out << "binary"; break; }
            default: { out << "unknown"; break; }
        }
        out << std::hex << "[0x"
            << static_cast<utree_type::fast_integral_type>(t) << "]";
        return out;
    }
    
    struct bad_type_exception : utree_exception
    {
        std::string msg;

        bad_type_exception(char const* error, utree_type::info got)
          : msg()
        {
            std::ostringstream oss;
            oss << "utree: " << error
                << " (got utree type '" << got << "')";
            msg = oss.str();
        }
        
        bad_type_exception(char const* error, utree_type::info got1,
                           utree_type::info got2)
          : msg()
        {
            std::ostringstream oss;
            oss << "utree: " << error
                << " (got utree types '" << got1 << "' and '" << got2 << "')";
            msg = oss.str();
        }

        virtual ~bad_type_exception() BOOST_NOEXCEPT_OR_NOTHROW {}

        virtual char const* what() const BOOST_NOEXCEPT_OR_NOTHROW
        { return msg.c_str(); }
    };
    
    struct empty_exception : utree_exception
    {
        char const* msg;

        empty_exception(char const* error) : msg(error) {}
        
        virtual ~empty_exception() BOOST_NOEXCEPT_OR_NOTHROW {}

        virtual char const* what() const BOOST_NOEXCEPT_OR_NOTHROW
        { return msg; }
    };

    ///////////////////////////////////////////////////////////////////////////
    // A typed string with parametric Base storage. The storage can be any
    // range or (stl container) of chars.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Base, utree_type::info type_>
    struct basic_string : Base
    {
        static utree_type::info const type = type_;

        basic_string()
          : Base() {}

        basic_string(Base const& base)
          : Base(base) {}

        template <typename Iterator>
        basic_string(Iterator bits, std::size_t len)
          : Base(bits, bits + len) {}

        template <typename Iterator>
        basic_string(Iterator first, Iterator last)
          : Base(first, last) {}

        basic_string& operator=(Base const& other)
        {
            Base::operator=(other);
            return *this;
        }
    };

    //[utree_strings
    /*`The `utree` string types described below are used by the `utree` API 
       only. These are not used to store information in the `utree` itself.
       Their purpose is to refer to different internal `utree` node types
       only. For instance, creating a `utree` from a binary data type will 
       create a `binary_type` utree node (see above).
    */
    /*`The binary data type can be represented either verbatim as a sequence 
       of bytes or as a pair of iterators into some other stored binary data 
       sequence. Use this string type to access/create a `binary_type` `utree`.
    */
    typedef basic_string<
        boost::iterator_range<char const*>, utree_type::binary_type
    > binary_range_type;
    typedef basic_string<
        std::string, utree_type::binary_type
    > binary_string_type;

    /*`The UTF-8 string can be represented either verbatim as a sequence of 
       characters or as a pair of iterators into some other stored binary data 
       sequence. Use this string type to access/create a `string_type` `utree`.
    */
    typedef basic_string<
        boost::iterator_range<char const*>, utree_type::string_type
    > utf8_string_range_type;
    typedef basic_string<
        std::string, utree_type::string_type
    > utf8_string_type;

    /*`The UTF-8 symbol can be represented either verbatim as a sequence of 
       characters or as a pair of iterators into some other stored binary data 
       sequence. Use this string type to access/create a `symbol_type` `utree`.
    */
    typedef basic_string<
        boost::iterator_range<char const*>, utree_type::symbol_type
    > utf8_symbol_range_type;
    typedef basic_string<
        std::string, utree_type::symbol_type
    > utf8_symbol_type;
    //]

    ///////////////////////////////////////////////////////////////////////////
    // Our function type
    ///////////////////////////////////////////////////////////////////////////
    class utree;

    //[utree_function_object_interface
    struct function_base
    {
        virtual ~function_base() {}
        virtual utree operator()(utree const& env) const = 0;
        virtual utree operator()(utree& env) const = 0;

        // Calling f.clone() must return a newly allocated function_base 
        // instance that is equal to f.
        virtual function_base* clone() const = 0; 
    };

    template <typename F>
    struct stored_function : function_base
    {
        F f;
        stored_function(F f = F());
        virtual ~stored_function();
        virtual utree operator()(utree const& env) const;
        virtual utree operator()(utree& env) const;
        virtual function_base* clone() const;
    };
    
    template <typename F>
    struct referenced_function : function_base
    {
        F& f;
        referenced_function(F& f);
        virtual ~referenced_function();
        virtual utree operator()(utree const& env) const;
        virtual utree operator()(utree& env) const;
        virtual function_base* clone() const;
    };
    //]

    ///////////////////////////////////////////////////////////////////////////
    // Shallow tag. Instructs utree to hold an iterator_range
    // as-is without deep copying the range.
    ///////////////////////////////////////////////////////////////////////////
    struct shallow_tag {};
    shallow_tag const shallow = {};

    ///////////////////////////////////////////////////////////////////////////
    // A void* plus type_info
    ///////////////////////////////////////////////////////////////////////////
    class any_ptr
    {
    public:
        template <typename Ptr>
        typename boost::disable_if<
            boost::is_polymorphic<
                typename boost::remove_pointer<Ptr>::type>,
            Ptr>::type
        get() const
        {
            if (*i == typeid(Ptr))
            {
                return static_cast<Ptr>(p);
            }
            boost::throw_exception(std::bad_cast());
        }

        template <typename T>
        any_ptr(T* p)
          : p(p), i(&typeid(T*))
        {}

        friend bool operator==(any_ptr const& a, any_ptr const& b)
        {
            return (a.p == b.p) && (*a.i == *b.i);
        }

    private:
        // constructor is private
        any_ptr(void* p, std::type_info const* i)
          : p(p), i(i) {}

        template <typename UTreeX, typename UTreeY>
        friend struct detail::visit_impl;

        friend class utree;

        void* p;
        std::type_info const* i;
    };

    //[utree
    class utree {
    public:
        ///////////////////////////////////////////////////////////////////////
        // The invalid type
        struct invalid_type {};

        ///////////////////////////////////////////////////////////////////////
        // The nil type
        struct nil_type {};

        ///////////////////////////////////////////////////////////////////////
        // The list type, this can be used to initialize an utree to hold an 
        // empty list
        struct list_type;

        //[utree_container_types
        typedef utree value_type;
        typedef utree& reference;
        typedef utree const& const_reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::size_t size_type;

        typedef detail::list::node_iterator<utree> iterator;
        typedef detail::list::node_iterator<utree const> const_iterator;
        //]

        typedef detail::list::node_iterator<boost::reference_wrapper<utree> >
          ref_iterator;

        typedef boost::iterator_range<iterator> range;
        typedef boost::iterator_range<const_iterator> const_range;

        // dtor
        ~utree();

        ////////////////////////////////////////////////////////////////////////
        //[utree_initialization
        /*`A `utree` can be constructed or initialized from a wide range of 
           data types, allowing to create `utree` instances for every 
           possible node type (see the description of `utree_type::info` above). 
           For this reason it exposes a constructor and an assignment operator 
           for each of the allowed node types as shown below. All constructors
           are non-explicit on purpose, allowing to use an utree instance as
           the attribute to almost any Qi parser.
        */
        // This constructs an `invalid_type` node. When used in places
        // where a boost::optional is expected (i.e. as an attribute for the 
        // optional component), this represents the 'empty' state.
        utree(invalid_type = invalid_type());

        // This initializes a `nil_type` node, which represents a valid,
        // 'initialized empty' utree (different from invalid_type!).
        utree(nil_type);
        reference operator=(nil_type);

        // This initializes a `boolean_type` node, which can hold 'true' or
        // 'false' only.
        explicit utree(bool);
        reference operator=(bool);

        // This initializes an `integer_type` node, which can hold arbitrary 
        // integers. For convenience these functions are overloaded for signed
        // and unsigned integer types.
        utree(unsigned int);
        utree(int);
        reference operator=(unsigned int);
        reference operator=(int);

        // This initializes a `double_type` node, which can hold arbitrary 
        // floating point (double) values.
        utree(double);
        reference operator=(double);

        // This initializes a `string_type` node, which can hold a narrow 
        // character sequence (usually an UTF-8 string).
        utree(char);
        utree(char const*);
        utree(char const*, std::size_t);
        utree(std::string const&);
        reference operator=(char);
        reference operator=(char const*);
        reference operator=(std::string const&);

        // This constructs a `string_range_type` node, which does not copy the 
        // data but stores the iterator range to the character sequence the 
        // range has been initialized from.
        utree(utf8_string_range_type const&, shallow_tag);

        // This initializes a `reference_type` node, which holds a reference to 
        // another utree node. All operations on such a node are automatically
        // forwarded to the referenced utree instance.
        utree(boost::reference_wrapper<utree>);
        reference operator=(boost::reference_wrapper<utree>);

        // This initializes an `any_type` node, which can hold a pointer to an
        // instance of any type together with the typeid of that type. When 
        // accessing that pointer the typeid will be checked, causing a 
        // std::bad_cast to be thrown if the typeids do not match.
        utree(any_ptr const&);
        reference operator=(any_ptr const&);

        // This initializes a `range_type` node, which holds an utree list node
        // the elements of which are copy constructed (assigned) from the 
        // elements referenced by the given range of iterators.
        template <class Iterator>
        utree(boost::iterator_range<Iterator>);
        template <class Iterator>
        reference operator=(boost::iterator_range<Iterator>);
        
        // This initializes a `function_type` node from a polymorphic function
        // object pointer (takes ownership) or reference. 
        utree(function_base const&);
        reference operator=(function_base const&);
        utree(function_base*);
        reference operator=(function_base*);

        // This initializes either a `string_type`, a `symbol_type`, or a 
        // `binary_type` node (depending on the template parameter `type_`), 
        // which will hold the corresponding narrow character sequence (usually 
        // an UTF-8 string).
        template <class Base, utree_type::info type_>
        utree(basic_string<Base, type_> const&);
        template <class Base, utree_type::info type_>
        reference operator=(basic_string<Base, type_> const&);
        //]

        // copy 
        utree(const_reference);
        reference operator=(const_reference);

        // range
        utree(range, shallow_tag);
        utree(const_range, shallow_tag);

        // assign dispatch
        template <class Iterator>
        void assign(Iterator, Iterator);

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // function object visitation interface

        // single dispatch
        template <class F>
        typename boost::result_of<F(utree const&)>::type
        static visit(utree const&, F);

        template <class F>
        typename boost::result_of<F(utree&)>::type
        static visit(utree&, F);

        // double dispatch
        template <class F>
        typename boost::result_of<F(utree const&, utree const&)>::type
        static visit(utree const&, utree const&, F);

        template <class F>
        typename boost::result_of<F(utree&, utree const&)>::type
        static visit(utree&, utree const&, F);

        template <class F>
        typename boost::result_of<F(utree const&, utree&)>::type
        static visit(utree const&, utree&, F);

        template <class F>
        typename boost::result_of<F(utree&, utree&)>::type
        static visit(utree&, utree&, F);

        ////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        //[utree_container_functions
        // STL Container interface

        // insertion 
        template <class T>
        void push_back(T const&);
        template <class T>
        void push_front(T const&);
        template <class T>
        iterator insert(iterator, T const&);
        template <class T>
        void insert(iterator, std::size_t, T const&);
        template <class Iterator>
        void insert(iterator, Iterator, Iterator);

        // erasure
        void pop_front();
        void pop_back();
        iterator erase(iterator);
        iterator erase(iterator, iterator);

        // front access
        reference front();
        const_reference front() const;
        iterator begin();
        const_iterator begin() const;
        ref_iterator ref_begin();

        // back access
        reference back();
        const_reference back() const;
        iterator end();
        const_iterator end() const;
        ref_iterator ref_end();
        //]

        // This clears the utree instance and resets its type to `invalid_type`
        void clear();

        void swap(utree&);
 
        bool empty() const;

        size_type size() const;
        /*`[warning `size()` has O(n) complexity on `utree` ranges. On utree
            lists, it has O(1) complexity.]`*/

        ////////////////////////////////////////////////////////////////////////

        //[utree_variant_functions
        // return the data type (`utree_type::info`) of the currently stored 
        // data item
        utree_type::info which() const;

        // access the currently stored data in a type safe manner, this will 
        // throw a `std::bad_cast()` if the currently stored data item is not 
        // default convertible to `T`.
        template <class T>
        T get() const;
        //]

        reference deref();
        const_reference deref() const;

        short tag() const;
        void tag(short);

        utree eval(utree const&) const;
        utree eval(utree&) const;

        utree operator() (utree const&) const;
        utree operator() (utree&) const;
    //<-
    protected:
        void ensure_list_type(char const* failed_in = "ensure_list_type()");

    private:
        typedef utree_type type;

        template <class UTreeX, class UTreeY>
        friend struct detail::visit_impl;
        friend struct detail::index_impl;

        type::info get_type() const;
        void set_type(type::info);
        void free();
        void copy(const_reference);

        union {
            detail::fast_string s;
            detail::list l;
            detail::range r;
            detail::string_range sr;
            detail::void_ptr v;
            bool b;
            int i;
            double d;
            utree* p;
            function_base* pf;
        };
    //->
    };
    //]

    //[utree_tuple_interface 
    /*<-*/inline/*->*/
    utree::reference get(utree::reference, utree::size_type);
    /*<-*/inline/*->*/
    utree::const_reference get(utree::const_reference, utree::size_type);
    /*`[warning `get()` has O(n) complexity.]`*/
    //]

    struct utree::list_type : utree
    {
        using utree::operator=;

        list_type() : utree() { ensure_list_type("list_type()"); }

        template <typename T0>
        list_type(T0 t0) : utree(t0) {}
      
        template <typename T0, typename T1>
        list_type(T0 t0, T1 t1) : utree(t0, t1) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // predefined instances for singular types
    utree::invalid_type const invalid = {};
    utree::nil_type const nil = {};
    utree::list_type const empty_list = utree::list_type();
}}

#if defined(BOOST_MSVC)
  #pragma warning(pop)
#endif

#endif

