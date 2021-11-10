#ifndef POSIX_TIME_SERIALIZE_HPP___
#define POSIX_TIME_SERIALIZE_HPP___

/* Copyright (c) 2004-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/greg_serialize.hpp"
#include "boost/core/nvp.hpp"
#include "boost/numeric/conversion/cast.hpp"
#include "boost/type_traits/integral_constant.hpp"

// Define versions for serialization compatibility
// alows the unit tests to make an older version to check compatibility
#ifndef BOOST_DATE_TIME_POSIX_TIME_DURATION_VERSION
#define BOOST_DATE_TIME_POSIX_TIME_DURATION_VERSION 1
#endif

namespace boost {
namespace serialization {

template<typename T>
struct version;

template<>
struct version<boost::posix_time::time_duration>
  : integral_constant<int, BOOST_DATE_TIME_POSIX_TIME_DURATION_VERSION>
{
};

// A macro to split serialize functions into save & load functions.
// It is here to avoid dependency on Boost.Serialization just for the
// BOOST_SERIALIZATION_SPLIT_FREE macro
#define BOOST_DATE_TIME_SPLIT_FREE(T)                                         \
template<class Archive>                                                       \
inline void serialize(Archive & ar,                                           \
                      T & t,                                                  \
                      const unsigned int file_version)                        \
{                                                                             \
  split_free(ar, t, file_version);                                            \
}

BOOST_DATE_TIME_SPLIT_FREE(boost::posix_time::ptime)
BOOST_DATE_TIME_SPLIT_FREE(boost::posix_time::time_duration)
BOOST_DATE_TIME_SPLIT_FREE(boost::posix_time::time_period)

#undef BOOST_DATE_TIME_SPLIT_FREE

/*** time_duration ***/

//! Function to save posix_time::time_duration objects using serialization lib
/*! time_duration objects are broken down into 4 parts for serialization:
 * types are hour_type, min_type, sec_type, and fractional_seconds_type
 * as defined in the time_duration class
 */
template<class TimeResTraitsSize, class Archive>
void save_td(Archive& ar, const posix_time::time_duration& td)
{
    TimeResTraitsSize h = boost::numeric_cast<TimeResTraitsSize>(td.hours());
    TimeResTraitsSize m = boost::numeric_cast<TimeResTraitsSize>(td.minutes());
    TimeResTraitsSize s = boost::numeric_cast<TimeResTraitsSize>(td.seconds());
    posix_time::time_duration::fractional_seconds_type fs = td.fractional_seconds();
    ar & make_nvp("time_duration_hours", h);
    ar & make_nvp("time_duration_minutes", m);
    ar & make_nvp("time_duration_seconds", s);
    ar & make_nvp("time_duration_fractional_seconds", fs);
}

template<class Archive>
void save(Archive & ar, 
          const posix_time::time_duration& td, 
          unsigned int version)
{
  // serialize a bool so we know how to read this back in later
  bool is_special = td.is_special();
  ar & make_nvp("is_special", is_special);
  if(is_special) {
    std::string s = to_simple_string(td);
    ar & make_nvp("sv_time_duration", s);
  }
  else {
    // Write support for earlier versions allows for upgrade compatibility testing
    // See load comments for version information
    if (version == 0) {
        save_td<int32_t>(ar, td);
    } else {
        save_td<int64_t>(ar, td);
    }
  }
}

//! Function to load posix_time::time_duration objects using serialization lib
/*! time_duration objects are broken down into 4 parts for serialization:
 * types are hour_type, min_type, sec_type, and fractional_seconds_type
 * as defined in the time_duration class
 */
template<class TimeResTraitsSize, class Archive>
void load_td(Archive& ar, posix_time::time_duration& td)
{
    TimeResTraitsSize h(0);
    TimeResTraitsSize m(0);
    TimeResTraitsSize s(0);
    posix_time::time_duration::fractional_seconds_type fs(0);
    ar & make_nvp("time_duration_hours", h);
    ar & make_nvp("time_duration_minutes", m);
    ar & make_nvp("time_duration_seconds", s);
    ar & make_nvp("time_duration_fractional_seconds", fs);
    td = posix_time::time_duration(h, m, s, fs);
}

template<class Archive>
void load(Archive & ar, 
          posix_time::time_duration & td, 
          unsigned int version)
{
  bool is_special = false;
  ar & make_nvp("is_special", is_special);
  if(is_special) {
    std::string s;
    ar & make_nvp("sv_time_duration", s);
    posix_time::special_values sv = gregorian::special_value_from_string(s);
    td = posix_time::time_duration(sv);
  }
  else {
    // Version "0"   (Boost 1.65.1 or earlier, which used int32_t for day/hour/minute/second and
    //                therefore suffered from the year 2038 issue.)
    // Version "0.5" (Boost 1.66.0 changed to std::time_t but did not increase the version;
    //                it was missed in the original change, all code reviews, and there were no
    //                static assertions to protect the code; further std::time_t can be 32-bit
    //                or 64-bit so it reduced portability.  This makes 1.66.0 hard to handle...)
    // Version "1"   (Boost 1.67.0 or later uses int64_t and is properly versioned)

    // If the size of any of these items changes, a new version is needed.
    BOOST_STATIC_ASSERT(sizeof(posix_time::time_duration::hour_type) == sizeof(boost::int64_t));
    BOOST_STATIC_ASSERT(sizeof(posix_time::time_duration::min_type) == sizeof(boost::int64_t));
    BOOST_STATIC_ASSERT(sizeof(posix_time::time_duration::sec_type) == sizeof(boost::int64_t));
    BOOST_STATIC_ASSERT(sizeof(posix_time::time_duration::fractional_seconds_type) == sizeof(boost::int64_t));

    if (version == 0) {
        load_td<int32_t>(ar, td);
    } else {
        load_td<int64_t>(ar, td);
    }
  }
}

// no load_construct_data function provided as time_duration provides a
// default constructor

/*** ptime ***/

//! Function to save posix_time::ptime objects using serialization lib
/*! ptime objects are broken down into 2 parts for serialization:
 * a date object and a time_duration onject
 */
template<class Archive>
void save(Archive & ar, 
          const posix_time::ptime& pt, 
          unsigned int /*version*/)
{
  // from_iso_string does not include fractional seconds
  // therefore date and time_duration are used
  posix_time::ptime::date_type d = pt.date();
  ar & make_nvp("ptime_date", d);
  if(!pt.is_special()) {
    posix_time::ptime::time_duration_type td = pt.time_of_day();
    ar & make_nvp("ptime_time_duration", td);
  }
}

//! Function to load posix_time::ptime objects using serialization lib
/*! ptime objects are broken down into 2 parts for serialization:
 * a date object and a time_duration onject
 */
template<class Archive>
void load(Archive & ar, 
          posix_time::ptime & pt, 
          unsigned int /*version*/)
{
  // from_iso_string does not include fractional seconds
  // therefore date and time_duration are used
  posix_time::ptime::date_type d(posix_time::not_a_date_time);
  posix_time::ptime::time_duration_type td;
  ar & make_nvp("ptime_date", d);
  if(!d.is_special()) {
    ar & make_nvp("ptime_time_duration", td);
    pt = boost::posix_time::ptime(d,td);
  }
  else {
    pt = boost::posix_time::ptime(d.as_special());
  }
    
}

//!override needed b/c no default constructor
template<class Archive>
inline void load_construct_data(Archive & /*ar*/, 
                                posix_time::ptime* pt, 
                                const unsigned int /*file_version*/)
{
  // retrieve data from archive required to construct new 
  // invoke inplace constructor to initialize instance of date
  new(pt) boost::posix_time::ptime(boost::posix_time::not_a_date_time);
}

/*** time_period ***/

//! Function to save posix_time::time_period objects using serialization lib
/*! time_period objects are broken down into 2 parts for serialization:
 * a begining ptime object and an ending ptime object
 */
template<class Archive>
void save(Archive & ar, 
          const posix_time::time_period& tp, 
          unsigned int /*version*/)
{
  posix_time::ptime beg(tp.begin().date(), tp.begin().time_of_day());
  posix_time::ptime end(tp.end().date(), tp.end().time_of_day());
  ar & make_nvp("time_period_begin", beg);
  ar & make_nvp("time_period_end", end);
}

//! Function to load posix_time::time_period objects using serialization lib
/*! time_period objects are broken down into 2 parts for serialization:
 * a begining ptime object and an ending ptime object
 */
template<class Archive>
void load(Archive & ar, 
          boost::posix_time::time_period & tp, 
          unsigned int /*version*/)
{
  posix_time::time_duration td(1,0,0);
  gregorian::date d(gregorian::not_a_date_time);
  posix_time::ptime beg(d,td);
  posix_time::ptime end(d,td);
  ar & make_nvp("time_period_begin", beg);
  ar & make_nvp("time_period_end", end);
  tp = boost::posix_time::time_period(beg, end);
}

//!override needed b/c no default constructor
template<class Archive>
inline void load_construct_data(Archive & /*ar*/, 
                                boost::posix_time::time_period* tp, 
                                const unsigned int /*file_version*/)
{
  posix_time::time_duration td(1,0,0);
  gregorian::date d(gregorian::not_a_date_time);
  posix_time::ptime beg(d,td);
  posix_time::ptime end(d,td);
  new(tp) boost::posix_time::time_period(beg,end);
}

} // namespace serialization
} // namespace boost

#endif
