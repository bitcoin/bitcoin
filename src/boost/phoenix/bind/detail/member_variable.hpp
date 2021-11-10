/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_DETAIL_MEMBER_VARIABLE_HPP
#define BOOST_PHOENIX_CORE_DETAIL_MEMBER_VARIABLE_HPP

#include <boost/proto/detail/decltype.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4180) // qualifier applied to function type has no meaning; ignored
#endif

namespace boost { namespace phoenix { namespace detail {

        template <typename RT, typename MP>
        struct member_variable
        {
            template <typename Sig>
            struct result;

            template <typename This, typename Class>
            struct result<This(Class)>
                : result<This(Class const &)>
            {};

            template <typename This, typename Class>
            struct result<This(Class &)>
            {
                typedef typename boost::mpl::if_c<
                    boost::is_const<
                        typename boost::remove_pointer<
                            typename boost::remove_reference<Class>::type
                        >::type
                    >::value
                  , const RT&
                  , RT&
                >::type
                type;
            };

            member_variable(MP mp_)
                : mp(mp_) {}

            template <typename Class>
            RT& operator()(Class& obj) const
            {
                BOOST_PROTO_USE_GET_POINTER();

                typedef typename proto::detail::class_member_traits<MP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*mp);
            }

            template <typename Class>
            RT& operator()(Class* obj) const
            {
                return obj->*mp;
            }

            template <typename Class>
            RT const& operator()(Class const& obj) const
            {
                BOOST_PROTO_USE_GET_POINTER();

                typedef typename proto::detail::class_member_traits<MP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*mp);
            }

            template <typename Class>
            RT const& operator()(Class const* obj) const
            {
                return obj->*mp;
            }

            MP mp;
        };
}}}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
