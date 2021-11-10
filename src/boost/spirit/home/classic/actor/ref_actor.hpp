/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_REF_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_REF_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy holder. This holder stores a reference to ref,
    //  act methods are fead with this reference. The parse result is not used
    //  by this holder.
    //
    //  (This doc uses convention available in actors.hpp)
    //
    //  Constructor:
    //      ...(T& ref_);
    //      where ref_ is stored.
    //
    //  Action calls:
    //      act(ref);
    //
    //  () operators: both
    //
    ///////////////////////////////////////////////////////////////////////////
    template<
        typename T,
        typename ActionT
    >
    class ref_actor : public ActionT
    {
    private:
        T& ref;
    public:
        explicit
        ref_actor(T& ref_)
        : ref(ref_){}


        template<typename T2>
        void operator()(T2 const& /*val*/) const
        {
            this->act(ref); // defined in ActionT
        }


        template<typename IteratorT>
        void operator()(
            IteratorT const& /*first*/,
            IteratorT const& /*last*/
            ) const
        {
            this->act(ref); // defined in ActionT
        }
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
