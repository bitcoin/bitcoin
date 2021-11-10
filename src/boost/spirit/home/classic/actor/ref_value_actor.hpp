/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    Copyright (c) 2011 Bryce Lelbach
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_REF_VALUE_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_REF_VALUE_ACTOR_HPP

#include <boost/detail/workaround.hpp>

#include <boost/spirit/home/classic/namespace.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy holder. This holder stores a reference to ref.
    //  act methods are feed with ref and the parse result.
    //
    //  (This doc uses convention available in actors.hpp)
    //
    //  Constructor:
    //      ...(T& ref_);
    //      where ref_ is stored.
    //
    //  Action calls:
    //      act(ref, value);
    //      act(ref, first,last);
    //
    //  () operators: both
    //
    ///////////////////////////////////////////////////////////////////////////
    template<
        typename T,
        typename ActionT
    >
    class ref_value_actor : public ActionT
    {
    private:
        T& ref;
    public:
        explicit
        ref_value_actor(T& ref_)
        : ref(ref_){}


        template<typename T2>
        void operator()(T2 const& val_) const
        {
            this->act(ref,val_); // defined in ActionT
        }


        template<typename IteratorT>
        void operator()(
            IteratorT const& first_,
            IteratorT const& last_
            ) const
        {
            this->act(ref,first_,last_); // defined in ActionT
        }
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

}}

#endif
