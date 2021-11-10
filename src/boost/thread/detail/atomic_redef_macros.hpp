// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#if defined(BOOST_INTEL)

#pragma pop_macro("atomic_compare_exchange")
#pragma pop_macro("atomic_compare_exchange_explicit")
#pragma pop_macro("atomic_exchange")
#pragma pop_macro("atomic_exchange_explicit")
#pragma pop_macro("atomic_is_lock_free")
#pragma pop_macro("atomic_load")
#pragma pop_macro("atomic_load_explicit")
#pragma pop_macro("atomic_store")
#pragma pop_macro("atomic_store_explicit")

#endif // #if defined(BOOST_INTEL)
