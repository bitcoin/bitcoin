#ifndef DATE_TIME_SPECIAL_DEFS_HPP__
#define DATE_TIME_SPECIAL_DEFS_HPP__

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 * $Date$
 */

namespace boost {
namespace date_time {

    enum special_values {not_a_date_time, 
                         neg_infin, pos_infin, 
                         min_date_time,  max_date_time,
                         not_special, NumSpecialValues};


} } //namespace date_time


#endif

