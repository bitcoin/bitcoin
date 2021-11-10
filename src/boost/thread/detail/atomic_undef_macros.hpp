// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#if defined(BOOST_INTEL)

#pragma push_macro("atomic_compare_exchange")
#undef atomic_compare_exchange

#pragma push_macro("atomic_compare_exchange_explicit")
#undef atomic_compare_exchange_explicit

#pragma push_macro("atomic_exchange")
#undef atomic_exchange

#pragma push_macro("atomic_exchange_explicit")
#undef atomic_exchange_explicit

#pragma push_macro("atomic_is_lock_free")
#undef atomic_is_lock_free

#pragma push_macro("atomic_load")
#undef atomic_load

#pragma push_macro("atomic_load_explicit")
#undef atomic_load_explicit

#pragma push_macro("atomic_store")
#undef atomic_store

#pragma push_macro("atomic_store_explicit")
#undef atomic_store_explicit


#endif // #if defined(BOOST_INTEL)


