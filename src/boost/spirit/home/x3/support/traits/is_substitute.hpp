/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_IS_SUBSTITUTE_JAN_9_2012_1049PM)
#define BOOST_SPIRIT_X3_IS_SUBSTITUTE_JAN_9_2012_1049PM

#include <boost/spirit/home/x3/support/traits/container_traits.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/value_at_key.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/count_if.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/optional/optional.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Find out if T can be a (strong) substitute for Attribute
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Attribute, typename Enable = void>
    struct is_substitute;

    template <typename Variant, typename Attribute>
    struct variant_has_substitute;

    namespace detail
    {
        template <typename T, typename Attribute>
        struct value_type_is_substitute
          : is_substitute<
                typename container_value<T>::type
              , typename container_value<Attribute>::type>
        {};

        template <typename T, typename Attribute, typename Enable = void>
        struct is_substitute_impl : mpl::false_ {};

        template <typename T, typename Attribute>
        struct is_substitute_impl<T, Attribute,
            typename enable_if<
                mpl::and_<
                    fusion::traits::is_sequence<T>,
                    fusion::traits::is_sequence<Attribute>,
                    mpl::equal<T, Attribute, is_substitute<mpl::_1, mpl::_2>>
                >
            >::type>
          : mpl::true_ {};

        template <typename T, typename Attribute>
        struct is_substitute_impl<T, Attribute,
            typename enable_if<
                mpl::and_<
                    is_container<T>,
                    is_container<Attribute>,
                    value_type_is_substitute<T, Attribute>
                >
            >::type>
          : mpl::true_ {};

        template <typename T, typename Attribute>
        struct is_substitute_impl<T, Attribute,
            typename enable_if<
                is_variant<Attribute>
            >::type>
          : variant_has_substitute<Attribute, T>
        {};
    }

    template <typename T, typename Attribute, typename Enable /*= void*/>
    struct is_substitute
        : mpl::or_<
              is_same<T, Attribute>,
              detail::is_substitute_impl<T, Attribute>> {};

    // for reference T
    template <typename T, typename Attribute, typename Enable>
    struct is_substitute<T&, Attribute, Enable>
        : is_substitute<T, Attribute, Enable> {};

    // for reference Attribute
    template <typename T, typename Attribute, typename Enable>
    struct is_substitute<T, Attribute&, Enable>
        : is_substitute<T, Attribute, Enable> {};

    // 2 element mpl tuple is compatible with fusion::map if:
    // - it's first element type is existing key in map
    // - it second element type is compatible to type stored at the key in map
    template <typename T, typename Attribute>
    struct is_substitute<T, Attribute
	, typename enable_if<
	      typename mpl::eval_if<
		  mpl::and_<fusion::traits::is_sequence<T>
			    , fusion::traits::is_sequence<Attribute>>
		  , mpl::and_<traits::has_size<T, 2>
			   , fusion::traits::is_associative<Attribute>>
		  , mpl::false_>::type>::type>

    {
        // checking that "p_key >> p_value" parser can
        // store it's result in fusion::map attribute
        typedef typename mpl::at_c<T, 0>::type p_key;
        typedef typename mpl::at_c<T, 1>::type p_value;

        // for simple p_key type we just check that
        // such key can be found in attr and that value under that key
        // matches p_value
        template <typename Key, typename Value, typename Map>
        struct has_kv_in_map
            : mpl::eval_if<
                fusion::result_of::has_key<Map, Key>
              , mpl::apply<
                    is_substitute<
                        fusion::result_of::value_at_key<mpl::_1, Key>
                      , Value>
                      , Map>
              , mpl::false_>
        {};

        // if p_key is variant over multiple types (as a result of
        // "(key1|key2|key3) >> p_value" parser) check that all
        // keys are found in fusion::map attribute and that values
        // under these keys match p_value
        template <typename Variant>
        struct variant_kv
            : mpl::equal_to<
                mpl::size< typename Variant::types>
              , mpl::size< mpl::filter_view<typename Variant::types
              , has_kv_in_map<mpl::_1, p_value, Attribute>>>
            >
        {};

        typedef typename
            mpl::eval_if<
                is_variant<p_key>
              , variant_kv<p_key>
              , has_kv_in_map<p_key, p_value, Attribute>
            >::type
        type;
    };

    template <typename T, typename Attribute>
    struct is_substitute<optional<T>, optional<Attribute>>
      : is_substitute<T, Attribute> {};
}}}}

#endif
