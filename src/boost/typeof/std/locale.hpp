// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_locale_hpp_INCLUDED
#define BOOST_TYPEOF_STD_locale_hpp_INCLUDED

#include <locale>
#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(std::locale)
BOOST_TYPEOF_REGISTER_TYPE(std::ctype_base)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::ctype, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::ctype_byname, 1)
BOOST_TYPEOF_REGISTER_TYPE(std::codecvt_base)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::codecvt, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::codecvt_byname, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::num_get, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::num_put, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::numpunct, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::numpunct_byname, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::collate, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::collate_byname, 1)
BOOST_TYPEOF_REGISTER_TYPE(std::time_base)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::time_get, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::time_get_byname, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::time_put, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::time_put_byname, 2)
BOOST_TYPEOF_REGISTER_TYPE(std::money_base)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::money_get, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::money_put, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::moneypunct, (class)(bool))
BOOST_TYPEOF_REGISTER_TEMPLATE(std::moneypunct_byname, (class)(bool))
BOOST_TYPEOF_REGISTER_TYPE(std::messages_base)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::messages, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::messages_byname, 1)

#endif//BOOST_TYPEOF_STD_locale_hpp_INCLUDED
