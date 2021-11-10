// ptr_list.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CONTAINERS_PTR_LIST_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CONTAINERS_PTR_LIST_HPP

#include <list>

namespace boost
{
namespace lexer
{
namespace detail
{
template<typename Type>
class ptr_list
{
public:
    typedef std::list<Type *> list;

    ptr_list ()
    {
    }

    ~ptr_list ()
    {
        clear ();
    }

    list *operator -> ()
    {
        return &_list;
    }

    const list *operator -> () const
    {
        return &_list;
    }

    list &operator * ()
    {
        return _list;
    }

    const list &operator * () const
    {
        return _list;
    }

    void clear ()
    {
        while (!_list.empty ())
        {
            delete _list.front ();
            _list.pop_front ();
        }
    }

private:
    list _list;

    ptr_list (const ptr_list &); // No copy construction.
    ptr_list &operator = (const ptr_list &); // No assignment.
};
}
}
}

#endif
