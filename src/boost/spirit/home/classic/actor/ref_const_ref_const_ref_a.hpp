/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_REF_CONST_REF_CONST_REF_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_REF_CONST_REF_CONST_REF_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy holder. This holder stores a reference to ref
    //  , a const reference to value1_ref and a const reference to value2_ref.
    //  Typically, value1_ref is a key and value2_ref is value for associative
    //  container operations.
    //  act methods are feed with ref, value1_ref, value2_ref. The parse result
    //  is not used by this holder.
    //
    //  (This doc uses convention available in actors.hpp)
    //
    //  Constructor:
    //      ...(
    //          T& ref_,
    //          Value1T const& value1_ref_,
    //          Value2T const& value2_ref_ );
    //      where ref_, value1_ref and value2_ref_ are stored in the holder.
    //
    //  Action calls:
    //      act(ref, value1_ref, value2_ref);
    //
    //  () operators: both
    //
    ///////////////////////////////////////////////////////////////////////////
    template<
        typename T,
        typename Value1T,
        typename Value2T,
        typename ActionT
    >
    class ref_const_ref_const_ref_actor : public ActionT
    {
    private:
        T& ref;
        Value1T const& value1_ref;
        Value2T const& value2_ref;
    public:
        ref_const_ref_const_ref_actor(
            T& ref_,
            Value1T const& value1_ref_,
            Value2T const& value2_ref_
            )
        :
            ref(ref_),
            value1_ref(value1_ref_),
            value2_ref(value2_ref_)
        {}


        template<typename T2>
        void operator()(T2 const& /*val*/) const
        {
            this->act(ref,value1_ref,value2_ref); // defined in ActionT
        }


        template<typename IteratorT>
            void operator()(
            IteratorT const& /*first*/,
            IteratorT const& /*last*/
            ) const
        {
            this->act(ref,value1_ref,value2_ref); // defined in ActionT
        }
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
