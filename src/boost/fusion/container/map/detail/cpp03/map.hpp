/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MAP_07212005_1106)
#define FUSION_MAP_07212005_1106

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/container/map/detail/cpp03/map_fwd.hpp>
#include <boost/fusion/container/map/detail/cpp03/at_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/value_at_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/begin_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/end_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/value_of_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/deref_data_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/deref_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/key_of_impl.hpp>
#include <boost/fusion/container/map/detail/cpp03/value_of_data_impl.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    defined(BOOST_MSVC) && (BOOST_MSVC == 1700)
// see map_forward_ctor.hpp
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#endif

#if !defined(BOOST_FUSION_DONT_USE_PREPROCESSED_FILES)
#include <boost/fusion/container/map/detail/cpp03/preprocessed/map.hpp>
#else
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/map" FUSION_MAX_MAP_SIZE_STR ".hpp")
#endif

/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

#define FUSION_HASH #

namespace boost { namespace fusion
{
    struct void_;
    struct fusion_sequence_tag;

    template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_MAP_SIZE, typename T)>
    struct map : sequence_base<map<BOOST_PP_ENUM_PARAMS(FUSION_MAX_MAP_SIZE, T)> >
    {
        struct category : random_access_traversal_tag, associative_tag {};

        typedef map_tag fusion_tag;
        typedef fusion_sequence_tag tag; // this gets picked up by MPL
        typedef mpl::false_ is_view;

        typedef vector<
            BOOST_PP_ENUM_PARAMS(FUSION_MAX_MAP_SIZE, T)>
        storage_type;

        typedef typename storage_type::size size;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map()
            : data() {}

        BOOST_FUSION_GPU_ENABLED
        map(map const& rhs)
            : data(rhs.data) {}

        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        map(Sequence const& rhs)
            : data(rhs) {}

        #include <boost/fusion/container/map/detail/cpp03/map_forward_ctor.hpp>

        template <typename T>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map& operator=(T const& rhs)
        {
            data = rhs;
            return *this;
        }

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map& operator=(map const& rhs)
        {
            data = rhs.data;
            return *this;
        }

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#endif
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    (defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES))
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map(map&& rhs)
            : data(std::move(rhs.data)) {}

        template <typename T>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map& operator=(T&& rhs)
        {
            data = BOOST_FUSION_FWD_ELEM(T, rhs);
            return *this;
        }

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map& operator=(map&& rhs)
        {
            data = std::move(rhs.data);
            return *this;
        }
#endif
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
FUSION_HASH endif
#endif

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        storage_type& get_data() { return data; }
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        storage_type const& get_data() const { return data; }

    private:

        storage_type data;
    };
}}

#undef FUSION_HASH

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif // BOOST_FUSION_DONT_USE_PREPROCESSED_FILES

#endif
