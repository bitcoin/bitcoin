/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ATTRIBUTE_TYPEOF_HPP)
#define BOOST_SPIRIT_ATTRIBUTE_TYPEOF_HPP

#include <boost/typeof/typeof.hpp>

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/typeof.hpp>
#include <boost/spirit/home/classic/attribute/closure_fwd.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    // parametric.hpp
    template<typename ChGenT>                        struct f_chlit;
    template<typename ChGenAT, typename ChGenBT>     struct f_range;
    template<typename IterGenAT, typename IterGenBT> class f_chseq;
    template<typename IterGenAT, typename IterGenBT> class f_strlit;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS


#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()


// parametric.hpp

BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::f_chlit,1)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::f_range,2)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::f_chseq,2)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::f_strlit,2)


// closure.hpp (has forward header)

BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::closure,BOOST_SPIRIT_CLOSURE_LIMIT)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::closure_context,1)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::init_closure_context,1)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::init_closure_parser,2)


#if BOOST_SPIRIT_CLOSURE_LIMIT  > 12
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::closure,12)
#endif
#if BOOST_SPIRIT_CLOSURE_LIMIT > 9
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::closure, 9)
#endif
#if BOOST_SPIRIT_CLOSURE_LIMIT > 6
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::closure, 6)
#endif
#if BOOST_SPIRIT_CLOSURE_LIMIT > 3
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::closure, 3)
#endif



#endif

