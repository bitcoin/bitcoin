//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_DATE_TIME_HPP_INCLUDED
#define BOOST_LOCALE_DATE_TIME_HPP_INCLUDED

#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif

#include <boost/locale/hold_ptr.hpp>
#include <boost/locale/date_time_facet.hpp>
#include <boost/locale/formatting.hpp>
#include <boost/locale/time_zone.hpp>
#include <locale>
#include <vector>
#include <stdexcept>


namespace boost {
    namespace locale {
        ///
        /// \defgroup date_time Date, Time, Timezone and Calendar manipulations 
        ///
        /// This module provides various calendar, timezone and date time services
        ///
        /// @{


        ///
        /// \brief This error is thrown in case of invalid state that occurred
        ///
        class BOOST_SYMBOL_VISIBLE date_time_error : public std::runtime_error {
        public:
            ///
            /// Constructor of date_time_error class
            /// 
            date_time_error(std::string const &e) : std::runtime_error(e) {}
        };


        ///
        /// \brief This class represents a pair of period_type and the integer
        /// values that describes its amount. For example 3 days or 4 years.
        ///
        /// Usually obtained as product of period_type and integer or
        /// my calling a representative functions
        /// For example day()*3 == date_time_period(day(),3) == day(3)
        /// 
        struct date_time_period 
        {
            period::period_type type;   ///< The type of period, i.e. era, year, day etc.
            int value;                  ///< The value the actual number of \a periods
            ///
            /// Operator + returns copy of itself
            ///
            date_time_period operator+() const { return *this; }
            ///
            /// Operator -, switches the sign of period
            ///
            date_time_period operator-() const { return date_time_period(type,-value); }
            
            ///
            /// Constructor that creates date_time_period from period_type \a f and a value \a v -- default 1.
            ///
            date_time_period(period::period_type f=period::period_type(),int v=1) : type(f), value(v) {}
        };

        namespace period {
            ///
            ///  Get period_type for: special invalid value, should not be used directly
            ///
            inline period_type invalid(){ return period_type(marks::invalid); }
            ///
            ///  Get period_type for: Era i.e. AC, BC in Gregorian and Julian calendar, range [0,1]
            ///
            inline period_type era(){ return period_type(marks::era); }
            ///
            ///  Get period_type for: Year, it is calendar specific, for example 2011 in Gregorian calendar.
            ///
            inline period_type year(){ return period_type(marks::year); }
            ///
            ///  Get period_type for: Extended year for Gregorian/Julian calendars, where 1 BC == 0, 2 BC == -1.
            ///
            inline period_type extended_year(){ return period_type(marks::extended_year); }
            ///
            ///  Get period_type for: The month of year, calendar specific, in Gregorian [0..11]
            ///
            inline period_type month(){ return period_type(marks::month); }
            ///
            ///  Get period_type for: The day of month, calendar specific, in Gregorian [1..31]
            ///
            inline period_type day(){ return period_type(marks::day); }
            ///
            ///  Get period_type for: The number of day in year, starting from 1, in Gregorian  [1..366]
            ///
            inline period_type day_of_year(){ return period_type(marks::day_of_year); }
            ///
            ///  Get period_type for: Day of week, Sunday=1, Monday=2,..., Saturday=7.
            ///
            /// Note that updating this value respects local day of week, so for example,
            /// If first day of week is Monday and the current day is Tuesday then setting
            /// the value to Sunday (1) would forward the date by 5 days forward and not backward
            /// by two days as it could be expected if the numbers were taken as is.
            ///
            inline period_type day_of_week(){ return period_type(marks::day_of_week); }
            ///
            ///  Get period_type for: Original number of the day of the week in month. For example 1st Sunday, 
            /// 2nd Sunday, etc. in Gregorian [1..5]
            ///
            inline period_type day_of_week_in_month(){ return period_type(marks::day_of_week_in_month); }
            ///
            ///  Get period_type for: Local day of week, for example in France Monday is 1, in US Sunday is 1, [1..7]
            ///
            inline period_type day_of_week_local(){ return period_type(marks::day_of_week_local); }
            ///
            ///  Get period_type for: 24 clock hour [0..23]
            ///
            inline period_type hour(){ return period_type(marks::hour); }
            ///
            ///  Get period_type for: 12 clock hour [0..11]
            ///
            inline period_type hour_12(){ return period_type(marks::hour_12); }
            ///
            ///  Get period_type for: am or pm marker [0..1]
            ///
            inline period_type am_pm(){ return period_type(marks::am_pm); }
            ///
            ///  Get period_type for: minute [0..59]
            ///
            inline period_type minute(){ return period_type(marks::minute); }
            ///
            ///  Get period_type for: second [0..59]
            ///
            inline period_type second(){ return period_type(marks::second); }
            ///
            ///  Get period_type for: The week number in the year
            ///
            inline period_type week_of_year(){ return period_type(marks::week_of_year); }
            ///
            ///  Get period_type for: The week number within current month
            ///
            inline period_type week_of_month(){ return period_type(marks::week_of_month); }
            ///
            ///  Get period_type for: First day of week, constant, for example Sunday in US = 1, Monday in France = 2
            ///
            inline period_type first_day_of_week(){ return period_type(marks::first_day_of_week); }

            ///
            ///  Get date_time_period for: Era i.e. AC, BC in Gregorian and Julian calendar, range [0,1]
            ///
            inline date_time_period era(int v) { return date_time_period(era(),v); } 
            ///
            ///  Get date_time_period for: Year, it is calendar specific, for example 2011 in Gregorian calendar.
            ///
            inline date_time_period year(int v) { return date_time_period(year(),v); } 
            ///
            ///  Get date_time_period for: Extended year for Gregorian/Julian calendars, where 1 BC == 0, 2 BC == -1.
            ///
            inline date_time_period extended_year(int v) { return date_time_period(extended_year(),v); } 
            ///
            ///  Get date_time_period for: The month of year, calendar specific, in Gregorian [0..11]
            ///
            inline date_time_period month(int v) { return date_time_period(month(),v); } 
            ///
            ///  Get date_time_period for: The day of month, calendar specific, in Gregorian [1..31]
            ///
            inline date_time_period day(int v) { return date_time_period(day(),v); } 
            ///
            ///  Get date_time_period for: The number of day in year, starting from 1, in Gregorian  [1..366]
            ///
            inline date_time_period day_of_year(int v) { return date_time_period(day_of_year(),v); } 
            ///
            ///  Get date_time_period for: Day of week, Sunday=1, Monday=2,..., Saturday=7.
            ///
            /// Note that updating this value respects local day of week, so for example,
            /// If first day of week is Monday and the current day is Tuesday then setting
            /// the value to Sunday (1) would forward the date by 5 days forward and not backward
            /// by two days as it could be expected if the numbers were taken as is.
            ///
            inline date_time_period day_of_week(int v) { return date_time_period(day_of_week(),v); } 
            ///
            ///  Get date_time_period for: Original number of the day of the week in month. For example 1st Sunday, 
            /// 2nd Sunday, etc. in Gregorian [1..5]
            ///
            inline date_time_period day_of_week_in_month(int v) { return date_time_period(day_of_week_in_month(),v); } 
            ///
            ///  Get date_time_period for: Local day of week, for example in France Monday is 1, in US Sunday is 1, [1..7]
            ///
            inline date_time_period day_of_week_local(int v) { return date_time_period(day_of_week_local(),v); } 
            ///
            ///  Get date_time_period for: 24 clock hour [0..23]
            ///
            inline date_time_period hour(int v) { return date_time_period(hour(),v); } 
            ///
            ///  Get date_time_period for: 12 clock hour [0..11]
            ///
            inline date_time_period hour_12(int v) { return date_time_period(hour_12(),v); } 
            ///
            ///  Get date_time_period for: am or pm marker [0..1]
            ///
            inline date_time_period am_pm(int v) { return date_time_period(am_pm(),v); } 
            ///
            ///  Get date_time_period for: minute [0..59]
            ///
            inline date_time_period minute(int v) { return date_time_period(minute(),v); } 
            ///
            ///  Get date_time_period for: second [0..59]
            ///
            inline date_time_period second(int v) { return date_time_period(second(),v); } 
            ///
            ///  Get date_time_period for: The week number in the year
            ///
            inline date_time_period week_of_year(int v) { return date_time_period(week_of_year(),v); } 
            ///
            ///  Get date_time_period for: The week number within current month
            ///
            inline date_time_period week_of_month(int v) { return date_time_period(week_of_month(),v); } 
            ///
            ///  Get date_time_period for: First day of week, constant, for example Sunday in US = 1, Monday in France = 2
            ///
            inline date_time_period first_day_of_week(int v) { return date_time_period(first_day_of_week(),v); } 

            ///
            /// Get predefined constant for January
            ///
            inline date_time_period january() { return date_time_period(month(),0); }
            ///
            /// Get predefined constant for February
            ///
            inline date_time_period february() { return date_time_period(month(),1); }
            ///
            /// Get predefined constant for March
            ///
            inline date_time_period march() { return date_time_period(month(),2); }
            ///
            /// Get predefined constant for April
            ///
            inline date_time_period april() { return date_time_period(month(),3); }
            ///
            /// Get predefined constant for May
            ///
            inline date_time_period may() { return date_time_period(month(),4); }
            ///
            /// Get predefined constant for June
            ///
            inline date_time_period june() { return date_time_period(month(),5); }
            ///
            /// Get predefined constant for July
            ///
            inline date_time_period july() { return date_time_period(month(),6); }
            ///
            /// Get predefined constant for August
            ///
            inline date_time_period august() { return date_time_period(month(),7); }
            ///
            /// Get predefined constant for September
            ///
            inline date_time_period september() { return date_time_period(month(),8); }
            ///
            /// Get predefined constant for October 
            ///
            inline date_time_period october() { return date_time_period(month(),9); }
            ///
            /// Get predefined constant for November
            ///
            inline date_time_period november() { return date_time_period(month(),10); }
            ///
            /// Get predefined constant for December
            ///
            inline date_time_period december() { return date_time_period(month(),11); }

            ///
            /// Get predefined constant for Sunday
            ///
            inline date_time_period sunday() { return date_time_period(day_of_week(),1); }
            ///
            /// Get predefined constant for Monday 
            ///
            inline date_time_period monday() { return date_time_period(day_of_week(),2); }
            ///
            /// Get predefined constant for Tuesday
            ///
            inline date_time_period tuesday() { return date_time_period(day_of_week(),3); }
            ///
            /// Get predefined constant for Wednesday
            ///
            inline date_time_period wednesday() { return date_time_period(day_of_week(),4); }
            ///
            /// Get predefined constant for Thursday
            ///
            inline date_time_period thursday() { return date_time_period(day_of_week(),5); }
            ///
            /// Get predefined constant for Friday
            ///
            inline date_time_period friday() { return date_time_period(day_of_week(),6); }
            ///
            /// Get predefined constant for Saturday
            ///
            inline date_time_period saturday() { return date_time_period(day_of_week(),7); }
            ///
            /// Get predefined constant for AM (Ante Meridiem)
            ///
            inline date_time_period am() { return date_time_period(am_pm(),0); }
            ///
            /// Get predefined constant for PM (Post Meridiem)
            ///
            inline date_time_period pm() { return date_time_period(am_pm(),1); }

            ///
            /// convert period_type to date_time_period(f,1)
            ///
            inline date_time_period operator+(period::period_type f) 
            {
                return date_time_period(f);
            }
            ///
            /// convert period_type to date_time_period(f,-1)
            ///
            inline date_time_period operator-(period::period_type f)
            {
                return date_time_period(f,-1);
            }

            ///
            /// Create date_time_period of type \a f with value \a v. 
            ///
            template<typename T>
            date_time_period operator*(period::period_type f,T v)
            {
                return date_time_period(f,v);
            }

            ///
            /// Create date_time_period of type \a f with value \a v. 
            ///
            template<typename T>
            date_time_period operator*(T v,period::period_type f)
            {
                return date_time_period(f,v);
            }
            ///
            /// Create date_time_period of type \a f with value \a v. 
            ///
            template<typename T>
            date_time_period operator*(T v,date_time_period f)
            {
                return date_time_period(f.type,f.value*v);
            }

            ///
            /// Create date_time_period of type \a f with value \a v. 
            ///
            template<typename T>
            date_time_period operator*(date_time_period f,T v)
            {
                return date_time_period(f.type,f.value*v);
            }


        } // period


        ///
        /// \brief this class that represents a set of periods, 
        ///
        /// It is generally created by operations on periods:
        /// 1995*year + 3*month + 1*day. Note: operations are not commutative.
        ///
        class date_time_period_set {
        public:
            
            ///
            /// Default constructor - empty set
            ///
            date_time_period_set()
            {
            }
            ///
            /// Create a set of single period with value 1
            ///
            date_time_period_set(period::period_type f)
            {
                basic_[0]=date_time_period(f);
            }
            ///
            /// Create a set of single period \a fl
            ///
            date_time_period_set(date_time_period const &fl)
            {
                basic_[0]=fl;
            }
            ///
            /// Append date_time_period \a f to the set
            ///
            void add(date_time_period f)
            {
                size_t n=size();
                if(n < 4)
                    basic_[n]=f;
                else
                    periods_.push_back(f);
            }
            ///
            /// Get number if items in list
            ///
            size_t size() const
            {
                if(basic_[0].type == period::period_type())
                    return 0;
                if(basic_[1].type == period::period_type())
                    return 1;
                if(basic_[2].type == period::period_type())
                    return 2;
                if(basic_[3].type == period::period_type())
                    return 3;
                return 4+periods_.size();
            }
            ///
            /// Get item at position \a n the set, n should be in range [0,size)
            ///
            date_time_period const &operator[](size_t n) const 
            {
                if(n >= size())
                    throw std::out_of_range("Invalid index to date_time_period");
                if(n < 4)
                    return basic_[n];
                else
                    return periods_[n-4];
            }
        private:
            date_time_period basic_[4];
            std::vector<date_time_period> periods_;
        };

        
        ///
        /// Append two periods sets. Note this operator is not commutative 
        ///
        inline date_time_period_set operator+(date_time_period_set const &a,date_time_period_set const &b)
        {
            date_time_period_set s(a);
            for(unsigned i=0;i<b.size();i++)
                s.add(b[i]);
            return s;
        }
        
        ///
        /// Append two period sets when all periods of set \b change their sign
        ///
        inline date_time_period_set operator-(date_time_period_set const &a,date_time_period_set const &b)
        {
            date_time_period_set s(a);
            for(unsigned i=0;i<b.size();i++)
                s.add(-b[i]);
            return s;
        }


        ///
        /// \brief this class provides an access to general calendar information. 
        ///
        /// This information is not connected to specific date but generic to locale, and timezone.
        /// It is used in obtaining general information about calendar and is essential for creation of
        /// date_time objects.
        ///
        class BOOST_LOCALE_DECL calendar {
        public:

            ///
            /// Create calendar taking locale and timezone information from ios_base instance.
            /// 
            /// \note throws std::bad_cast if ios does not have a locale with installed \ref calendar_facet
            /// facet installed
            /// 
            calendar(std::ios_base &ios);
            ///
            /// Create calendar with locale \a l and time_zone \a zone
            ///
            /// \note throws std::bad_cast if loc does not have \ref calendar_facet facet installed
            /// 
            calendar(std::locale const &l,std::string const &zone);
            ///
            /// Create calendar with locale \a l and default timezone
            ///
            /// \note throws std::bad_cast if loc does not have \ref calendar_facet facet installed
            /// 
            calendar(std::locale const &l);
            ///
            /// Create calendar with default locale and timezone \a zone
            ///
            /// \note throws std::bad_cast if global locale does not have \ref calendar_facet facet installed
            /// 
            calendar(std::string const &zone);
            ///
            /// Create calendar with default locale and timezone 
            ///
            /// \note throws std::bad_cast if global locale does not have \ref calendar_facet facet installed
            /// 
            calendar();
            ~calendar();

            ///
            /// copy calendar
            ///
            calendar(calendar const &other);
            ///
            /// assign calendar
            ///
            calendar const &operator=(calendar const &other);

            ///
            /// Get minimum value for period f, For example for period::day it is 1.
            ///
            int minimum(period::period_type f) const;
            ///
            /// Get greatest possible minimum value for period f, For example for period::day it is 1, but may be different for other calendars.
            ///
            int greatest_minimum(period::period_type f) const;
            ///
            /// Get maximum value for period f, For example for Gregorian calendar's maximum period::day it is 31.
            ///
            int maximum(period::period_type f) const;
            ///
            /// Get least maximum value for period f, For example for Gregorian calendar's maximum period::day it is 28.
            ///
            int least_maximum(period::period_type f) const;

            ///
            /// Get first day of week for specific calendar, for example for US it is 1 - Sunday for France it is 2 - Monday
            int first_day_of_week() const;

            ///
            /// get calendar's locale
            ///
            std::locale get_locale() const;
            ///
            /// get calendar's time zone
            ///
            std::string get_time_zone() const;

            ///
            /// Check if the calendar is Gregorian
            ///
            bool is_gregorian() const;

            ///
            /// Compare calendars for equivalence: i.e. calendar types, time zones etc.
            ///
            bool operator==(calendar const &other) const;
            ///
            /// Opposite of ==
            ///
            bool operator!=(calendar const &other) const;

        private:
            friend class date_time;
            std::locale locale_;
            std::string tz_;
            hold_ptr<abstract_calendar> impl_;
        };

        ///
        /// \brief this class represents a date time and allows to perform various operation according to the
        /// locale settings.
        ///
        /// This class allows to manipulate various aspects of dates and times easily using arithmetic operations with
        /// periods.
        ///
        /// General arithmetic functions:
        ///
        /// - date_time + date_time_period_set = date_time: move time point forward by specific periods like date_time + month;
        /// - date_time - date_time_period_set = date_time: move time point backward by specific periods like date_time - month;
        /// - date_time << date_time_period_set  = date_time: roll time point forward by specific periods with rolling to begin if overflows: like "2010-01-31" << 2* day == "2010-01-02" instead of "2010-02-02"
        /// - date_time >> date_time_period_set  = date_time: roll time point backward by specific periods with rolling to end if overflows: like "2010-01-02" >> 2* day == "2010-01-31" instead of "2009-12-30"
        /// - date_time / period_type = int - current period value: like "2010-12-21" / month == 12. "2010-12-21" / year = 2010
        /// - (date_time - date_time) / period_type = int: distance between dates in period_type. Like ("2010-12-01" - "2008-12-01") / month = 24.
        ///
        /// You can also assign specific periods using assignment operator like:
        /// some_time = year * 1995 that sets the year to 1995.
        ///
        ///
        
        class BOOST_LOCALE_DECL date_time {
        public:

            ///
            /// Dafault constructor, uses default calendar initialized date_time object to current time.
            ///
            /// \note throws std::bad_cast if the global locale does not have \ref calendar_facet facet installed
            /// 
            date_time();
            ///
            /// copy date_time
            ///
            date_time(date_time const &other);
            ///
            /// copy date_time and change some fields according to the \a set
            ///
            date_time(date_time const &other,date_time_period_set const &set);
            ///
            /// assign the date_time
            ///
            date_time const &operator=(date_time const &other);
            ~date_time();

            ///
            /// Create a date_time object using POSIX time \a time and default calendar
            ///
            /// \note throws std::bad_cast if the global locale does not have \ref calendar_facet facet installed
            /// 
            date_time(double time);
            ///
            /// Create a date_time object using POSIX time \a time and calendar \a cal
            ///
            date_time(double time,calendar const &cal);
            ///
            /// Create a date_time object using calendar \a cal and initializes it to current time.
            ///
            date_time(calendar const &cal);
            
            ///
            /// Create a date_time object using default calendar and define values given in \a set
            ///
            /// \note throws std::bad_cast if the global locale does not have \ref calendar_facet facet installed
            /// 
            date_time(date_time_period_set const &set);
            ///
            /// Create a date_time object using calendar \a cal and define values given in \a set
            ///
            date_time(date_time_period_set const &set,calendar const &cal);

           
            ///
            /// assign values to various periods in set \a f  
            ///
            date_time const &operator=(date_time_period_set const &f);

            ///
            /// set specific period \a f value to \a v
            ///
            void set(period::period_type f,int v);
            ///
            /// get specific period \a f value
            ///
            int get(period::period_type f) const;

            ///
            /// syntactic sugar for get(f)
            ///
            int operator/(period::period_type f) const
            {
                return get(f);
            }

            ///
            /// add single period f to the current date_time
            ///
            date_time operator+(period::period_type f) const
            {
                return *this+date_time_period(f);
            }

            ///
            /// subtract single period f from the current date_time
            ///
            date_time operator-(period::period_type f) const
            {
                return *this-date_time_period(f);
            }

            ///
            /// add single period f to the current date_time
            ///
            date_time const &operator+=(period::period_type f)
            {
                return *this+=date_time_period(f);
            }
            ///
            /// subtract single period f from the current date_time
            ///
            date_time const &operator-=(period::period_type f)
            {
                return *this-=date_time_period(f);
            }

            ///
            /// roll forward a date by single period f.
            ///
            date_time operator<<(period::period_type f) const
            {
                return *this<<date_time_period(f);
            }

            ///
            /// roll backward a date by single period f.
            ///
            date_time operator>>(period::period_type f) const
            {
                return *this>>date_time_period(f);
            }

            ///
            /// roll forward a date by single period f.
            ///
            date_time const &operator<<=(period::period_type f)
            {
                return *this<<=date_time_period(f);
            }
            ///
            /// roll backward a date by single period f.
            ///
            date_time const &operator>>=(period::period_type f)
            {
                return *this>>=date_time_period(f);
            }

            ///
            /// add date_time_period to the current date_time
            ///
            date_time operator+(date_time_period const &v) const;
            ///
            /// subtract date_time_period from the current date_time
            ///
            date_time operator-(date_time_period const &v) const;
            ///
            /// add date_time_period to the current date_time
            ///
            date_time const &operator+=(date_time_period const &v);
            ///
            /// subtract date_time_period from the current date_time
            ///
            date_time const &operator-=(date_time_period const &v);

            ///
            /// roll current date_time forward by date_time_period v
            ///
            date_time operator<<(date_time_period const &v) const;
            ///
            /// roll current date_time backward by date_time_period v
            ///
            date_time operator>>(date_time_period const &v) const ;
            ///
            /// roll current date_time forward by date_time_period v
            ///
            date_time const &operator<<=(date_time_period const &v);
            ///
            /// roll current date_time backward by date_time_period v
            ///
            date_time const &operator>>=(date_time_period const &v);

            ///
            /// add date_time_period_set v to the current date_time
            ///
            date_time operator+(date_time_period_set const &v) const;
            ///
            /// subtract date_time_period_set v from the current date_time
            ///
            date_time operator-(date_time_period_set const &v) const;
            ///
            /// add date_time_period_set v to the current date_time
            ///
            date_time const &operator+=(date_time_period_set const &v);
            ///
            /// subtract date_time_period_set v from the current date_time
            ///
            date_time const &operator-=(date_time_period_set const &v);

            ///
            /// roll current date_time forward by date_time_period_set v
            ///
            date_time operator<<(date_time_period_set const &v) const;
            ///
            /// roll current date_time backward by date_time_period_set v
            ///
            date_time operator>>(date_time_period_set const &v) const ;
            ///
            /// roll current date_time forward by date_time_period_set v
            ///
            date_time const &operator<<=(date_time_period_set const &v);
            ///
            /// roll current date_time backward by date_time_period_set v
            ///
            date_time const &operator>>=(date_time_period_set const &v);

            ///
            /// Get POSIX time
            ///
            /// The POSIX time is number of seconds since January 1st, 1970 00:00 UTC, ignoring leap seconds.
            ///
            double time() const;
            ///
            /// set POSIX time
            ///
            /// The POSIX time is number of seconds since January 1st, 1970 00:00 UTC, ignoring leap seconds.
            /// This time can be fetched from Operating system clock using C function time, gettimeofday and others.
            ///
            void time(double v);

            ///
            /// compare date_time in the timeline (ignores difference in calendar, timezone etc)
            ///
            bool operator==(date_time const &other) const;
            ///
            /// compare date_time in the timeline (ignores difference in calendar, timezone etc)
            ///
            bool operator!=(date_time const &other) const;
            ///
            /// compare date_time in the timeline (ignores difference in calendar, timezone etc)
            ///
            bool operator<(date_time const &other) const;
            ///
            /// compare date_time in the timeline (ignores difference in calendar, timezone etc)
            ///
            bool operator>(date_time const &other) const;
            ///
            /// compare date_time in the timeline (ignores difference in calendar, timezone etc)
            ///
            bool operator<=(date_time const &other) const;
            ///
            /// compare date_time in the timeline (ignores difference in calendar, timezone etc)
            ///
            bool operator>=(date_time const &other) const;

            ///
            /// swaps two dates - efficient, does not throw
            ///
            void swap(date_time &other);

            ///
            /// calculate the distance from this date_time to \a other in terms of perios \a f
            ///
            int difference(date_time const &other,period::period_type f) const;

            ///
            /// Get minimal possible value for *this time point for a period \a f.
            ///
            int minimum(period::period_type f) const;
            ///
            /// Get minimal possible value for *this time point for a period \a f. For example
            /// in February maximum(day) may be 28 or 29, in January maximum(day)==31
            ///
            int maximum(period::period_type f) const;

            ///
            /// Check if *this time point is in daylight saving time 
            ///
            bool is_in_daylight_saving_time() const;

        private:
            hold_ptr<abstract_calendar> impl_;
        };

        ///
        /// Writes date_time \a t to output stream \a out.
        ///
        /// This function uses locale, calendar and time zone of the target stream \a in.
        ///
        /// For example:
        /// \code
        ///  date_time now(time(0),hebrew_calendar)
        ///  cout << "Year: " << period::year(now) <<" Full Date:"<< now;
        /// \endcode
        ///
        /// The output may be Year:5770 Full Date:Jan 1, 2010
        /// 
        template<typename CharType>
        std::basic_ostream<CharType> &operator<<(std::basic_ostream<CharType> &out,date_time const &t)
        {
            double time_point = t.time();
            uint64_t display_flags = ios_info::get(out).display_flags();
            if  (
                    display_flags == flags::date 
                    || display_flags == flags::time 
                    || display_flags == flags::datetime 
                    || display_flags == flags::strftime
                ) 
            {
                out << time_point;
            }
            else {
                ios_info::get(out).display_flags(flags::datetime);
                out << time_point;
                ios_info::get(out).display_flags(display_flags);
            }
            return out;
        }

        ///
        /// Reads date_time \a t from output stream \a in
        ///
        /// This function uses locale, calendar and time zone of the source stream \a in.
        ///
        template<typename CharType>
        std::basic_istream<CharType> &operator>>(std::basic_istream<CharType> &in,date_time &t)
        {
            double v;
            uint64_t display_flags = ios_info::get(in).display_flags();
            if  (
                    display_flags == flags::date 
                    || display_flags == flags::time 
                    || display_flags == flags::datetime 
                    || display_flags == flags::strftime
                ) 
            {
                in >> v;
            }
            else {
                ios_info::get(in).display_flags(flags::datetime);
                in >> v;
                ios_info::get(in).display_flags(display_flags);
            }
            if(!in.fail())
                t.time(v);
            return in;
        }

        ///
        /// \brief This class represents a period: a pair of two date_time objects.
        /// 
        /// It is generally used as syntactic sugar to calculate difference between two dates.
        ///
        /// Note: it stores references to the original objects, so it is not recommended to be used
        /// outside of the equation you calculate the difference in.
        ///
        class date_time_duration {
        public:

            ///
            /// Create an object were \a first represents earlier point on time line and \a second is later
            /// point.
            /// 
            date_time_duration(date_time const &first,date_time const &second) :
                s_(first),
                e_(second)
            {
            }
            
            ///
            /// find a difference in terms of period_type \a f
            ///
            int get(period::period_type f) const
            {
                return start().difference(end(),f);
            }

            ///
            /// Syntactic sugar for get(f)
            ///
            int operator / (period::period_type f) const
            {
                return start().difference(end(),f);
            }

            ///
            /// Get starting point
            ///
            date_time const &start() const { return s_; }
            ///
            /// Get ending point
            ///
            date_time const &end() const { return e_; }
        private:
            date_time const &s_;
            date_time const &e_;
        };

        ///
        /// Calculates the difference between two dates, the left operand is a later point on time line.
        /// Returns date_time_duration object.
        ///
        inline date_time_duration operator-(date_time const &later,date_time const &earlier)
        {
            return date_time_duration(earlier,later);
        }

        
        namespace period {
            ///
            ///  Extract from date_time numerical value of Era i.e. AC, BC in Gregorian and Julian calendar, range [0,1]
            ///
            inline int era(date_time const &dt) { return dt.get(era()); } 
            ///
            ///  Extract from date_time numerical value of Year, it is calendar specific, for example 2011 in Gregorian calendar.
            ///
            inline int year(date_time const &dt) { return dt.get(year()); } 
            ///
            ///  Extract from date_time numerical value of Extended year for Gregorian/Julian calendars, where 1 BC == 0, 2 BC == -1.
            ///
            inline int extended_year(date_time const &dt) { return dt.get(extended_year()); } 
            ///
            ///  Extract from date_time numerical value of The month of year, calendar specific, in Gregorian [0..11]
            ///
            inline int month(date_time const &dt) { return dt.get(month()); } 
            ///
            ///  Extract from date_time numerical value of The day of month, calendar specific, in Gregorian [1..31]
            ///
            inline int day(date_time const &dt) { return dt.get(day()); } 
            ///
            ///  Extract from date_time numerical value of The number of day in year, starting from 1, in Gregorian  [1..366]
            ///
            inline int day_of_year(date_time const &dt) { return dt.get(day_of_year()); } 
            ///
            ///  Extract from date_time numerical value of Day of week, Sunday=1, Monday=2,..., Saturday=7.
            ///
            /// Note that updating this value respects local day of week, so for example,
            /// If first day of week is Monday and the current day is Tuesday then setting
            /// the value to Sunday (1) would forward the date by 5 days forward and not backward
            /// by two days as it could be expected if the numbers were taken as is.
            ///
            inline int day_of_week(date_time const &dt) { return dt.get(day_of_week()); } 
            ///
            ///  Extract from date_time numerical value of Original number of the day of the week in month. For example 1st Sunday, 
            /// 2nd Sunday, etc. in Gregorian [1..5]
            ///
            inline int day_of_week_in_month(date_time const &dt) { return dt.get(day_of_week_in_month()); } 
            ///
            ///  Extract from date_time numerical value of Local day of week, for example in France Monday is 1, in US Sunday is 1, [1..7]
            ///
            inline int day_of_week_local(date_time const &dt) { return dt.get(day_of_week_local()); } 
            ///
            ///  Extract from date_time numerical value of 24 clock hour [0..23]
            ///
            inline int hour(date_time const &dt) { return dt.get(hour()); } 
            ///
            ///  Extract from date_time numerical value of 12 clock hour [0..11]
            ///
            inline int hour_12(date_time const &dt) { return dt.get(hour_12()); } 
            ///
            ///  Extract from date_time numerical value of am or pm marker [0..1]
            ///
            inline int am_pm(date_time const &dt) { return dt.get(am_pm()); } 
            ///
            ///  Extract from date_time numerical value of minute [0..59]
            ///
            inline int minute(date_time const &dt) { return dt.get(minute()); } 
            ///
            ///  Extract from date_time numerical value of second [0..59]
            ///
            inline int second(date_time const &dt) { return dt.get(second()); } 
            ///
            ///  Extract from date_time numerical value of The week number in the year
            ///
            inline int week_of_year(date_time const &dt) { return dt.get(week_of_year()); } 
            ///
            ///  Extract from date_time numerical value of The week number within current month
            ///
            inline int week_of_month(date_time const &dt) { return dt.get(week_of_month()); } 
            ///
            ///  Extract from date_time numerical value of First day of week, constant, for example Sunday in US = 1, Monday in France = 2
            ///
            inline int first_day_of_week(date_time const &dt) { return dt.get(first_day_of_week()); } 
            
            ///
            ///  Extract from date_time_duration numerical value of duration in  Era i.e. AC, BC in Gregorian and Julian calendar, range [0,1]
            ///
            inline int era(date_time_duration const &dt) { return dt.get(era()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in years
            ///
            inline int year(date_time_duration const &dt) { return dt.get(year()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in extended years (for Gregorian/Julian calendars, where 1 BC == 0, 2 BC == -1).
            ///
            inline int extended_year(date_time_duration const &dt) { return dt.get(extended_year()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in months
            ///
            inline int month(date_time_duration const &dt) { return dt.get(month()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in days of month
            ///
            inline int day(date_time_duration const &dt) { return dt.get(day()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in days of year
            ///
            inline int day_of_year(date_time_duration const &dt) { return dt.get(day_of_year()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in days of week
            ///
            inline int day_of_week(date_time_duration const &dt) { return dt.get(day_of_week()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in original number of the day of the week in month
            ///
            inline int day_of_week_in_month(date_time_duration const &dt) { return dt.get(day_of_week_in_month()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in local day of week
            ///
            inline int day_of_week_local(date_time_duration const &dt) { return dt.get(day_of_week_local()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in hours
            ///
            inline int hour(date_time_duration const &dt) { return dt.get(hour()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in  12 clock hours
            ///
            inline int hour_12(date_time_duration const &dt) { return dt.get(hour_12()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in  am or pm markers
            ///
            inline int am_pm(date_time_duration const &dt) { return dt.get(am_pm()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in  minutes
            ///
            inline int minute(date_time_duration const &dt) { return dt.get(minute()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in  seconds
            ///
            inline int second(date_time_duration const &dt) { return dt.get(second()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in the week number in the year
            ///
            inline int week_of_year(date_time_duration const &dt) { return dt.get(week_of_year()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in  The week number within current month
            ///
            inline int week_of_month(date_time_duration const &dt) { return dt.get(week_of_month()); } 
            ///
            ///  Extract from date_time_duration numerical value of duration in the first day of week
            ///
            inline int first_day_of_week(date_time_duration const &dt) { return dt.get(first_day_of_week()); } 


        }
        
        /// @}


    } // locale
} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


#endif
///
/// \example calendar.cpp
///
/// Example of using date_time functions for generating calendar for current year.
///

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
