/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_UTREE_DETAIL1)
#define BOOST_SPIRIT_UTREE_DETAIL1

#include <boost/type_traits/alignment_of.hpp>

namespace boost { namespace spirit { namespace detail
{
    template <typename UTreeX, typename UTreeY>
    struct visit_impl;

    struct index_impl;

    ///////////////////////////////////////////////////////////////////////////
    // Our POD double linked list. Straightforward implementation.
    // This implementation is very primitive and is not meant to be
    // used stand-alone. This is the internal data representation
    // of lists in our utree.
    ///////////////////////////////////////////////////////////////////////////
    struct list // keep this a POD!
    {
        struct node;

        template <typename Value>
        class node_iterator;

        void free();
        void copy(list const& other);
        void default_construct();

        template <typename T, typename Iterator>
        void insert(T const& val, Iterator pos);

        template <typename T>
        void push_front(T const& val);

        template <typename T>
        void push_back(T const& val);

        void pop_front();
        void pop_back();
        node* erase(node* pos);

        node* first;
        node* last;
        std::size_t size;
    };

    ///////////////////////////////////////////////////////////////////////////
    // A range of utree(s) using an iterator range (begin/end) of node(s)
    ///////////////////////////////////////////////////////////////////////////
    struct range
    {
        list::node* first;
        list::node* last;
    };

    ///////////////////////////////////////////////////////////////////////////
    // A range of char*s
    ///////////////////////////////////////////////////////////////////////////
    struct string_range
    {
        char const* first;
        char const* last;
    };

    ///////////////////////////////////////////////////////////////////////////
    // A void* plus type_info
    ///////////////////////////////////////////////////////////////////////////
    struct void_ptr
    {
        void* p;
        std::type_info const* i;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Our POD fast string. This implementation is very primitive and is not
    // meant to be used stand-alone. This is the internal data representation
    // of strings in our utree. This is deliberately a POD to allow it to be
    // placed in a union. This POD fast string specifically utilizes
    // (sizeof(list) * alignment_of(list)) - (2 * sizeof(char)). In a 32 bit
    // system, this is 14 bytes. The two extra bytes are used by utree to store
    // management info.
    //
    // It is a const string (i.e. immutable). It stores the characters directly
    // if possible and only uses the heap if the string does not fit. Null
    // characters are allowed, making it suitable to encode raw binary. The
    // string length is encoded in the first byte if the string is placed in-situ,
    // else, the length plus a pointer to the string in the heap are stored.
    ///////////////////////////////////////////////////////////////////////////
    struct fast_string // Keep this a POD!
    {
        static std::size_t const
            buff_size = (sizeof(list) + boost::alignment_of<list>::value)
                / sizeof(char);

        static std::size_t const
            small_string_size = buff_size-sizeof(char);

        static std::size_t const
            max_string_len = small_string_size - 3;

        struct heap_store
        {
            char* str;
            std::size_t size;
        };

        union
        {
            char buff[buff_size];
            long lbuff[buff_size / (sizeof(long)/sizeof(char))];   // for initialize 
            heap_store heap;
        };

        int get_type() const;
        void set_type(int t);
        bool is_heap_allocated() const;

        std::size_t size() const;
        char const* str() const;

        template <typename Iterator>
        void construct(Iterator f, Iterator l);

        void swap(fast_string& other);
        void free();
        void copy(fast_string const& other);
        void initialize();

        char& info();
        char info() const;

        short tag() const;
        void tag(short tag);
    };
}}}

#endif
