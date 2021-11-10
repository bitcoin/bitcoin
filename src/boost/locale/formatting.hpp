//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_FORMATTING_HPP_INCLUDED
#define BOOST_LOCALE_FORMATTING_HPP_INCLUDED

#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <boost/cstdint.hpp>
#include <boost/locale/time_zone.hpp>
#include <ostream>
#include <istream>
#include <string>
#include <string.h>
#include <typeinfo>

namespace boost {
    namespace locale {
        ///
        /// \brief This namespace holds additional formatting
        /// flags that can be set using ios_info.
        ///
        namespace flags {
            ///
            /// Formatting flags, each one of them has corresponding manipulation
            /// in namespace \a as
            ///
            typedef enum {
                posix               = 0,
                number              = 1,
                currency            = 2,
                percent             = 3,
                date                = 4,
                time                = 5,
                datetime            = 6,
                strftime            = 7,
                spellout            = 8,
                ordinal             = 9,

                display_flags_mask  = 31,

                currency_default    = 0 << 5,
                currency_iso        = 1 << 5,
                currency_national   = 2 << 5,

                currency_flags_mask = 3 << 5,

                time_default        = 0 << 7,
                time_short          = 1 << 7,
                time_medium         = 2 << 7,
                time_long           = 3 << 7,
                time_full           = 4 << 7,
                time_flags_mask     = 7 << 7,

                date_default        = 0 << 10,
                date_short          = 1 << 10,
                date_medium         = 2 << 10,
                date_long           = 3 << 10,
                date_full           = 4 << 10,
                date_flags_mask     = 7 << 10,

                datetime_flags_mask = date_flags_mask | time_flags_mask

            } display_flags_type;

            ///
            /// Special string patters that can be used
            /// for text formatting
            ///
            typedef enum {
                datetime_pattern,   ///< strftime like formatting
                time_zone_id        ///< time zone name
            } pattern_type;

            ///
            /// Special integer values that can be used for formatting
            ///
            typedef enum {
                domain_id           ///< Domain code - for message formatting
            } value_type;

            
        } // flags

        ///
        /// \brief This class holds an external data - beyond existing fmtflags that std::ios_base holds
        ///
        /// You should almost never create this object directly. Instead, you should access it via ios_info::get(stream_object)
        /// static member function. It automatically creates default formatting data for that stream
        ///
        class BOOST_LOCALE_DECL ios_info {
        public:

            /// \cond INTERNAL

            ios_info();
            ios_info(ios_info const &);
            ios_info const &operator=(ios_info const &);
            ~ios_info();

            /// \endcond

            ///
            /// Get ios_info instance for specific stream object
            ///
            static ios_info &get(std::ios_base &ios);

            ///
            /// Set a flags that define a way for format data like number, spell, currency etc.
            ///
            void display_flags(uint64_t flags);
            
            ///
            /// Set a flags that define how to format currency
            ///
            void currency_flags(uint64_t flags);
            
            ///
            /// Set a flags that define how to format date
            ///
            void date_flags(uint64_t flags);
            
            ///
            /// Set a flags that define how to format time
            ///
            void time_flags(uint64_t flags);
            
            ///
            /// Set a flags that define how to format both date and time
            ///
            void datetime_flags(uint64_t flags);
            
            ///
            /// Set special message domain identification
            ///
            void domain_id(int);
            
            ///
            /// Set time zone for formatting dates and time
            ///
            void time_zone(std::string const &);
            

            ///
            /// Set date/time pattern (strftime like)
            ///
            template<typename CharType>
            void date_time_pattern(std::basic_string<CharType> const &str)
            {
                string_set &s = date_time_pattern_set();
                s.set<CharType>(str.c_str());
            }


            ///
            /// Get a flags that define a way for format data like number, spell, currency etc.
            ///
            uint64_t display_flags() const;
            
            ///
            /// Get a flags that define how to format currency
            ///
            uint64_t currency_flags() const;

            
            ///
            /// Get a flags that define how to format date
            ///
            uint64_t date_flags() const;
            
            ///
            /// Get a flags that define how to format time
            ///
            uint64_t time_flags() const;

            ///
            /// Get a flags that define how to format both date and time
            ///
            uint64_t datetime_flags() const;
            
            ///
            /// Get special message domain identification
            ///
            int domain_id() const;
            
            ///
            /// Get time zone for formatting dates and time
            ///
            std::string time_zone() const;
            
            ///
            /// Get date/time pattern (strftime like)
            ///
            template<typename CharType>
            std::basic_string<CharType> date_time_pattern() const
            {
                string_set const &s = date_time_pattern_set();
                return s.get<CharType>();
            }
            
            /// \cond INTERNAL
            void on_imbue();
            /// \endcond
            
        private:

            class string_set;

            string_set const &date_time_pattern_set() const;
            string_set &date_time_pattern_set();
            
            class BOOST_LOCALE_DECL string_set {
            public:
                string_set(); 
                ~string_set();
                string_set(string_set const &other);
                string_set const &operator=(string_set const &other);
                void swap(string_set &other);
                
                template<typename Char>
                void set(Char const *s)
                {
                    delete [] ptr;
                    ptr = 0;
                    type=&typeid(Char);
                    Char const *end = s;
                    while(*end!=0) end++;
                    // if ptr = 0 it does not matter what is value of size
                    size = sizeof(Char)*(end - s+1);
                    ptr = new char[size];
                    memcpy(ptr,s,size);
                }

                template<typename Char>
                std::basic_string<Char> get() const
                {
                    if(type==0 || *type!=typeid(Char))
                        throw std::bad_cast();
                    std::basic_string<Char> result = reinterpret_cast<Char const *>(ptr);
                    return result;
                }

            private:
                std::type_info const *type;
                size_t size;
                char *ptr;
            };

            uint64_t flags_;
            int domain_id_;
            std::string time_zone_;
            string_set datetime_;

            struct data;
            data *d;

        };


        ///
        /// \brief This namespace includes all manipulators that can be used on IO streams
        ///
        namespace as {
            ///
            /// \defgroup manipulators I/O Stream manipulators
            ///
            /// @{
            ///

            ///
            /// Format values with "POSIX" or "C"  locale. Note, if locale was created with additional non-classic locale then
            /// These numbers may be localized
            ///
            
            inline std::ios_base & posix(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::posix);
                return ios;
            }

            ///
            /// Format a number. Note, unlike standard number formatting, integers would be treated like real numbers when std::fixed or
            /// std::scientific manipulators were applied
            ///
            inline std::ios_base & number(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::number);
                return ios;
            }
            
            ///
            /// Format currency, number is treated like amount of money
            ///
            inline std::ios_base & currency(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::currency);
                return ios;
            }
            
            ///
            /// Format percent, value 0.3 is treated as 30%.
            ///
            inline std::ios_base & percent(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::percent);
                return ios;
            }
            
            ///
            /// Format a date, number is treated as POSIX time
            ///
            inline std::ios_base & date(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::date);
                return ios;
            }

            ///
            /// Format a time, number is treated as POSIX time
            ///
            inline std::ios_base & time(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::time);
                return ios;
            }

            ///
            /// Format a date and time, number is treated as POSIX time
            ///
            inline std::ios_base & datetime(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::datetime);
                return ios;
            }

            ///
            /// Create formatted date time, Please note, this manipulator only changes formatting mode,
            /// and not format itself, so you are probably looking for ftime manipulator
            ///
            inline std::ios_base & strftime(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::strftime);
                return ios;
            }
            
            ///
            /// Spell the number, like "one hundred and ten"
            ///
            inline std::ios_base & spellout(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::spellout);
                return ios;
            }
            
            ///
            /// Write an order of the number like 4th.
            ///
            inline std::ios_base & ordinal(std::ios_base & ios)
            {
                ios_info::get(ios).display_flags(flags::ordinal);
                return ios;
            }

            ///
            /// Set default currency formatting style -- national, like "$"
            ///
            inline std::ios_base & currency_default(std::ios_base & ios)
            {
                ios_info::get(ios).currency_flags(flags::currency_default);
                return ios;
            }

            ///
            /// Set ISO currency formatting style, like "USD", (requires ICU >= 4.2)
            ///
            inline std::ios_base & currency_iso(std::ios_base & ios)
            {
                ios_info::get(ios).currency_flags(flags::currency_iso);
                return ios;
            }

            ///
            /// Set national currency formatting style, like "$"
            ///
            inline std::ios_base & currency_national(std::ios_base & ios)
            {
                ios_info::get(ios).currency_flags(flags::currency_national);
                return ios;
            }

            ///
            /// set default (medium) time formatting style
            ///
            inline std::ios_base & time_default(std::ios_base & ios)
            {
                ios_info::get(ios).time_flags(flags::time_default);
                return ios;
            }

            ///
            /// set short time formatting style
            ///
            inline std::ios_base & time_short(std::ios_base & ios)
            {
                ios_info::get(ios).time_flags(flags::time_short);
                return ios;
            }

            ///
            /// set medium time formatting style
            ///
            inline std::ios_base & time_medium(std::ios_base & ios)
            {
                ios_info::get(ios).time_flags(flags::time_medium);
                return ios;
            }

            ///
            /// set long time formatting style
            ///
            inline std::ios_base & time_long(std::ios_base & ios)
            {
                ios_info::get(ios).time_flags(flags::time_long);
                return ios;
            }

            ///
            /// set full time formatting style
            ///
            inline std::ios_base & time_full(std::ios_base & ios)
            {
                ios_info::get(ios).time_flags(flags::time_full);
                return ios;
            }

            ///
            /// set default (medium) date formatting style
            ///
            inline std::ios_base & date_default(std::ios_base & ios)
            {
                ios_info::get(ios).date_flags(flags::date_default);
                return ios;
            }

            ///
            /// set short date formatting style
            ///
            inline std::ios_base & date_short(std::ios_base & ios)
            {
                ios_info::get(ios).date_flags(flags::date_short);
                return ios;
            }

            ///
            /// set medium date formatting style
            ///
            inline std::ios_base & date_medium(std::ios_base & ios)
            {
                ios_info::get(ios).date_flags(flags::date_medium);
                return ios;
            }

            ///
            /// set long date formatting style
            ///
            inline std::ios_base & date_long(std::ios_base & ios)
            {
                ios_info::get(ios).date_flags(flags::date_long);
                return ios;
            }

            ///
            /// set full date formatting style
            ///
            inline std::ios_base & date_full(std::ios_base & ios)
            {
                ios_info::get(ios).date_flags(flags::date_full);
                return ios;
            }            
            
            
            /// \cond INTERNAL 
            namespace details {
                template<typename CharType>
                struct add_ftime {

                    std::basic_string<CharType> ftime;

                    void apply(std::basic_ios<CharType> &ios) const
                    {
                        ios_info::get(ios).date_time_pattern(ftime);
                        as::strftime(ios);
                    }

                };

                template<typename CharType>
                std::basic_ostream<CharType> &operator<<(std::basic_ostream<CharType> &out,add_ftime<CharType> const &fmt)
                {
                    fmt.apply(out);
                    return out;
                }
                
                template<typename CharType>
                std::basic_istream<CharType> &operator>>(std::basic_istream<CharType> &in,add_ftime<CharType> const &fmt)
                {
                    fmt.apply(in);
                    return in;
                }

            }
            /// \endcond 

            ///
            /// Set strftime like formatting string
            ///
            /// Please note, formatting flags are very similar but not exactly the same as flags for C function strftime.
            /// Differences: some flags as "%e" do not add blanks to fill text up to two spaces, not all flags supported.
            ///
            /// Flags:
            /// -   "%a" -- Abbreviated  weekday (Sun.)
            /// -   "%A" -- Full weekday (Sunday)
            /// -   "%b" -- Abbreviated month (Jan.)
            /// -   "%B" -- Full month (January)
            /// -   "%c" -- Locale date-time format. **Note:** prefer using "as::datetime"
            /// -   "%d" -- Day of Month [01,31]
            /// -   "%e" -- Day of Month [1,31]
            /// -   "%h" -- Same as "%b"
            /// -   "%H" -- 24 clock hour [00,23]
            /// -   "%I" -- 12 clock hour [01,12]
            /// -   "%j" -- Day of year [1,366]
            /// -   "%m" -- Month [01,12]
            /// -   "%M" -- Minute [00,59]
            /// -   "%n" -- New Line
            /// -   "%p" -- AM/PM in locale representation
            /// -   "%r" -- Time with AM/PM, same as "%I:%M:%S %p"
            /// -   "%R" -- Same as "%H:%M"
            /// -   "%S" -- Second [00,61]
            /// -   "%t" -- Tab character
            /// -   "%T" -- Same as "%H:%M:%S"
            /// -   "%x" -- Local date representation. **Note:** prefer using "as::date"
            /// -   "%X" -- Local time representation. **Note:** prefer using "as::time"
            /// -   "%y" -- Year [00,99]
            /// -   "%Y" -- 4 digits year. (2009)
            /// -   "%Z" -- Time Zone
            /// -   "%%" -- Percent symbol
            ///


            template<typename CharType>
            #ifdef BOOST_LOCALE_DOXYGEN
            unspecified_type
            #else
            details::add_ftime<CharType> 
            #endif
            ftime(std::basic_string<CharType> const &format)
            {
                details::add_ftime<CharType> fmt;
                fmt.ftime=format;
                return fmt;
            }

            ///
            /// See ftime(std::basic_string<CharType> const &format)
            ///
            template<typename CharType>
            #ifdef BOOST_LOCALE_DOXYGEN
            unspecified_type
            #else
            details::add_ftime<CharType> 
            #endif
            ftime(CharType const *format)
            {
                details::add_ftime<CharType> fmt;
                fmt.ftime=format;
                return fmt;
            }

            /// \cond INTERNAL
            namespace details {
                struct set_timezone {
                    std::string id;
                };
                template<typename CharType>
                std::basic_ostream<CharType> &operator<<(std::basic_ostream<CharType> &out,set_timezone const &fmt)
                {
                    ios_info::get(out).time_zone(fmt.id);
                    return out;
                }
                
                template<typename CharType>
                std::basic_istream<CharType> &operator>>(std::basic_istream<CharType> &in,set_timezone const &fmt)
                {
                    ios_info::get(in).time_zone(fmt.id);
                    return in;
                }
            }
            /// \endcond
            
            ///
            /// Set GMT time zone to stream
            /// 
            inline std::ios_base &gmt(std::ios_base &ios)
            {
                ios_info::get(ios).time_zone("GMT");
                return ios;
            }

            ///
            /// Set local time zone to stream
            ///
            inline std::ios_base &local_time(std::ios_base &ios)
            {
                ios_info::get(ios).time_zone(time_zone::global());
                return ios;
            }

            ///
            /// Set time zone using \a id
            ///
            inline 
            #ifdef BOOST_LOCALE_DOXYGEN
            unspecified_type
            #else
            details::set_timezone 
            #endif
            time_zone(char const *id) 
            {
                details::set_timezone tz;
                tz.id=id;
                return tz;
            }

            ///
            /// Set time zone using \a id
            ///
            inline 
            #ifdef BOOST_LOCALE_DOXYGEN
            unspecified_type
            #else
            details::set_timezone 
            #endif            
            time_zone(std::string const &id) 
            {
                details::set_timezone tz;
                tz.id=id;
                return tz;
            }


        ///
        /// @}
        ///

        } // as manipulators
        
    } // locale
} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
