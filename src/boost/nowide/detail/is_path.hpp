//
//  Copyright (c) 2020 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_DETAIL_IS_PATH_HPP_INCLUDED
#define BOOST_NOWIDE_DETAIL_IS_PATH_HPP_INCLUDED

#include <type_traits>

namespace boost {
namespace nowide {
    namespace detail {

        /// Trait to heuristically check for a *\::filesystem::path
        /// Done by checking for make_preferred and filename member functions with correct signature
        template<typename T>
        struct is_path
        {
            template<typename U, U& (U::*)(), U (U::*)() const>
            struct Check;
            template<typename U>
            static std::true_type test(Check<U, &U::make_preferred, &U::filename>*);
            template<typename U>
            static std::false_type test(...);

            static constexpr bool value = decltype(test<T>(0))::value;
        };
        /// SFINAE trait which has a member "type = Result" if the Path is a *\::filesystem::path
        template<typename Path, typename Result>
        using enable_if_path_t = typename std::enable_if<is_path<Path>::value, Result>::type;

    } // namespace detail
} // namespace nowide
} // namespace boost

#endif
