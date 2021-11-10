//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_QI_DETAIL_ATTRIBUTES_HPP
#define BOOST_SPIRIT_QI_DETAIL_ATTRIBUTES_HPP

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/support/attributes_fwd.hpp>
#include <boost/spirit/home/support/attributes.hpp>
#include <boost/spirit/home/support/utree/utree_traits_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{
    template <typename Exposed, typename Transformed>
    struct default_transform_attribute
    {
        typedef Transformed type;

        static Transformed pre(Exposed&) { return Transformed(); }

        static void post(Exposed& val, Transformed const& attr)
        {
            traits::assign_to(attr, val);
        }

        // fail() will be called by Qi rule's if the rhs failed parsing
        static void fail(Exposed&) {}
    };

    // handle case where no transformation is required as the types are the same
    template <typename Attribute>
    struct default_transform_attribute<Attribute, Attribute>
    {
        typedef Attribute& type;
        static Attribute& pre(Attribute& val) { return val; }
        static void post(Attribute&, Attribute const&) {}
        static void fail(Attribute&) {}
    };

    template <typename Exposed, typename Transformed>
    struct proxy_transform_attribute
    {
        typedef Transformed type;

        static Transformed pre(Exposed& val) { return Transformed(val); }
        static void post(Exposed&, Transformed const&) { /* no-op */ }

        // fail() will be called by Qi rule's if the rhs failed parsing
        static void fail(Exposed&) {}
    };

    // handle case where no transformation is required as the types are the same
    template <typename Attribute>
    struct proxy_transform_attribute<Attribute, Attribute>
    {
        typedef Attribute& type;
        static Attribute& pre(Attribute& val) { return val; }
        static void post(Attribute&, Attribute const&) {}
        static void fail(Attribute&) {}
    };

    // main specialization for Qi
    template <typename Exposed, typename Transformed, typename Enable = void>
    struct transform_attribute
      : mpl::if_<
            mpl::and_<
                mpl::not_<is_const<Exposed> >
              , mpl::not_<is_reference<Exposed> >
              , traits::is_proxy<Transformed> >
          , proxy_transform_attribute<Exposed, Transformed>
          , default_transform_attribute<Exposed, Transformed> 
        >::type 
    {};

    template <typename Exposed, typename Transformed>
    struct transform_attribute<boost::optional<Exposed>, Transformed
      , typename disable_if<is_same<boost::optional<Exposed>, Transformed> >::type>
    {
        typedef Transformed& type;
        static Transformed& pre(boost::optional<Exposed>& val)
        {
            if (!val)
                val = Transformed();
            return boost::get<Transformed>(val);
        }
        static void post(boost::optional<Exposed>&, Transformed const&) {}
        static void fail(boost::optional<Exposed>& val)
        {
             val = none;    // leave optional uninitialized if rhs failed
        }
    };

    // unused_type needs some special handling as well
    template <>
    struct transform_attribute<unused_type, unused_type>
    {
        typedef unused_type type;
        static unused_type pre(unused_type) { return unused; }
        static void post(unused_type, unused_type) {}
        static void fail(unused_type) {}
    };

    template <>
    struct transform_attribute<unused_type const, unused_type>
      : transform_attribute<unused_type, unused_type>
    {};

    template <typename Attribute>
    struct transform_attribute<Attribute, unused_type>
      : transform_attribute<unused_type, unused_type>
    {};

    template <typename Attribute>
    struct transform_attribute<Attribute const, unused_type>
      : transform_attribute<unused_type, unused_type>
    {};
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    namespace detail {
        template <typename Exposed, typename Transformed>
        struct transform_attribute_base<Exposed, Transformed, qi::domain>
          : qi::transform_attribute<Exposed, Transformed>
        {};
    }
}}}

#endif
