
//  chrono_io
//
//  (C) Copyright Howard Hinnant
//  (C) Copyright 2010 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
// This code was adapted by Vicente from Howard Hinnant's experimental work
// on chrono i/o under lvm/libc++  to Boost

#ifndef BOOST_CHRONO_IO_V1_CHRONO_IO_HPP
#define BOOST_CHRONO_IO_V1_CHRONO_IO_HPP

#include <boost/chrono/chrono.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <boost/chrono/thread_clock.hpp>
#include <boost/chrono/clock_string.hpp>
#include <boost/ratio/ratio_io.hpp>
#include <locale>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/mpl/if.hpp>
#include <boost/integer/common_factor_rt.hpp>
#include <boost/chrono/detail/scan_keyword.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/chrono/detail/no_warning/signed_unsigned_cmp.hpp>

namespace boost
{

namespace chrono
{

template <class CharT>
class duration_punct
    : public std::locale::facet
{
public:
    typedef std::basic_string<CharT> string_type;
    enum {use_long, use_short};

private:
    bool use_short_;
    string_type long_seconds_;
    string_type long_minutes_;
    string_type long_hours_;
    string_type short_seconds_;
    string_type short_minutes_;
    string_type short_hours_;

    template <class Period>
        string_type short_name(Period) const
            {return ::boost::ratio_string<Period, CharT>::short_name() + short_seconds_;}

    string_type short_name(ratio<1>) const    {return short_seconds_;}
    string_type short_name(ratio<60>) const   {return short_minutes_;}
    string_type short_name(ratio<3600>) const {return short_hours_;}

    template <class Period>
        string_type long_name(Period) const
            {return ::boost::ratio_string<Period, CharT>::long_name() + long_seconds_;}

    string_type long_name(ratio<1>) const    {return long_seconds_;}
    string_type long_name(ratio<60>) const   {return long_minutes_;}
    string_type long_name(ratio<3600>) const {return long_hours_;}

    void init_C();
public:
    static std::locale::id id;

    explicit duration_punct(int use = use_long)
        : use_short_(use==use_short) {init_C();}

    duration_punct(int use,
        const string_type& long_seconds, const string_type& long_minutes,
        const string_type& long_hours, const string_type& short_seconds,
        const string_type& short_minutes, const string_type& short_hours);

    duration_punct(int use, const duration_punct& d);

    template <class Period>
        string_type short_name() const
            {return short_name(typename Period::type());}

    template <class Period>
        string_type long_name() const
            {return long_name(typename Period::type());}

    template <class Period>
        string_type plural() const
            {return long_name(typename Period::type());}

    template <class Period>
        string_type singular() const
    {
      return string_type(long_name(typename Period::type()), 0, long_name(typename Period::type()).size()-1);
    }

    template <class Period>
        string_type name() const
    {
      if (use_short_) return short_name<Period>();
      else {
        return long_name<Period>();
      }
    }
    template <class Period, class D>
      string_type name(D v) const
      {
        if (use_short_) return short_name<Period>();
        else
        {
          if (v==-1 || v==1)
            return singular<Period>();
          else
            return plural<Period>();
        }
      }

    bool is_short_name() const {return use_short_;}
    bool is_long_name() const {return !use_short_;}
};

template <class CharT>
std::locale::id
duration_punct<CharT>::id;

template <class CharT>
void
duration_punct<CharT>::init_C()
{
    short_seconds_ = CharT('s');
    short_minutes_ = CharT('m');
    short_hours_ = CharT('h');
    const CharT s[] = {'s', 'e', 'c', 'o', 'n', 'd', 's'};
    const CharT m[] = {'m', 'i', 'n', 'u', 't', 'e', 's'};
    const CharT h[] = {'h', 'o', 'u', 'r', 's'};
    long_seconds_.assign(s, s + sizeof(s)/sizeof(s[0]));
    long_minutes_.assign(m, m + sizeof(m)/sizeof(m[0]));
    long_hours_.assign(h, h + sizeof(h)/sizeof(h[0]));
}

template <class CharT>
duration_punct<CharT>::duration_punct(int use,
        const string_type& long_seconds, const string_type& long_minutes,
        const string_type& long_hours, const string_type& short_seconds,
        const string_type& short_minutes, const string_type& short_hours)
    : use_short_(use==use_short),
      long_seconds_(long_seconds),
      long_minutes_(long_minutes),
      long_hours_(long_hours),
      short_seconds_(short_seconds),
      short_minutes_(short_minutes),
      short_hours_(short_hours)
{}

template <class CharT>
duration_punct<CharT>::duration_punct(int use, const duration_punct& d)
    : use_short_(use==use_short),
      long_seconds_(d.long_seconds_),
      long_minutes_(d.long_minutes_),
      long_hours_(d.long_hours_),
      short_seconds_(d.short_seconds_),
      short_minutes_(d.short_minutes_),
      short_hours_(d.short_hours_)
{}

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
duration_short(std::basic_ostream<CharT, Traits>& os)
{
    typedef duration_punct<CharT> Facet;
    std::locale loc = os.getloc();
    if (std::has_facet<Facet>(loc))
    {
        const Facet& f = std::use_facet<Facet>(loc);
        if (f.is_long_name())
            os.imbue(std::locale(loc, new Facet(Facet::use_short, f)));
    }
    else
        os.imbue(std::locale(loc, new Facet(Facet::use_short)));
    return os;
}

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
duration_long(std::basic_ostream<CharT, Traits>& os)
{
    typedef duration_punct<CharT> Facet;
    std::locale loc = os.getloc();
    if (std::has_facet<Facet>(loc))
    {
        const Facet& f = std::use_facet<Facet>(loc);
        if (f.is_short_name())
            os.imbue(std::locale(loc, new Facet(Facet::use_long, f)));
    }
    return os;
}

template <class CharT, class Traits, class Rep, class Period>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const duration<Rep, Period>& d)
{
    typedef duration_punct<CharT> Facet;
    std::locale loc = os.getloc();
    if (!std::has_facet<Facet>(loc))
        os.imbue(std::locale(loc, new Facet));
    const Facet& f = std::use_facet<Facet>(os.getloc());
    return os << d.count() << ' ' << f.template name<Period>(d.count());
}

namespace chrono_detail {
template <class Rep, bool = is_scalar<Rep>::value>
struct duration_io_intermediate
{
    typedef Rep type;
};

template <class Rep>
struct duration_io_intermediate<Rep, true>
{
    typedef typename mpl::if_c
    <
        is_floating_point<Rep>::value,
            long double,
            typename mpl::if_c
            <
                is_signed<Rep>::value,
                    long long,
                    unsigned long long
            >::type
    >::type type;
};

template <typename intermediate_type>
typename enable_if<is_integral<intermediate_type>, bool>::type
reduce(intermediate_type& r, unsigned long long& den, std::ios_base::iostate& err)
{
  typedef typename common_type<intermediate_type, unsigned long long>::type common_type_t;

    // Reduce r * num / den
  common_type_t t = integer::gcd<common_type_t>(common_type_t(r), common_type_t(den));
  r /= t;
  den /= t;
  if (den != 1)
  {
    // Conversion to Period is integral and not exact
    err |= std::ios_base::failbit;
    return false;
  }
  return true;
}
template <typename intermediate_type>
typename disable_if<is_integral<intermediate_type>, bool>::type
reduce(intermediate_type& , unsigned long long& , std::ios_base::iostate& )
{
  return true;
}

}

template <class CharT, class Traits, class Rep, class Period>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is, duration<Rep, Period>& d)
{
  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
    typedef duration_punct<CharT> Facet;
    std::locale loc = is.getloc();
    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
    if (!std::has_facet<Facet>(loc)) {
      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        is.imbue(std::locale(loc, new Facet));
    }
    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
    loc = is.getloc();
    const Facet& f = std::use_facet<Facet>(loc);
    typedef typename chrono_detail::duration_io_intermediate<Rep>::type intermediate_type;
    intermediate_type r;
    std::ios_base::iostate err = std::ios_base::goodbit;
    // read value into r
    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
    is >> r;
    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
    if (is.good())
    {
      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        // now determine unit
        typedef std::istreambuf_iterator<CharT, Traits> in_iterator;
        in_iterator i(is);
        in_iterator e;
        //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        if (i != e && *i == ' ')  // mandatory ' ' after value
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
            ++i;
            if (i != e)
            {
              //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                // unit is num / den (yet to be determined)
                unsigned long long num = 0;
                unsigned long long den = 0;
                if (*i == '[')
                {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    // parse [N/D]s or [N/D]seconds format
                    ++i;
                    CharT x;
                    is >> num >> x >> den;
                    if (!is.good() || (x != '/'))
                    {
                      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                        is.setstate(is.failbit);
                        return is;
                    }
                    i = in_iterator(is);
                    if (*i != ']')
                    {
                      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                        is.setstate(is.failbit);
                        return is;
                    }
                    ++i;
                    const std::basic_string<CharT> units[] =
                    {
                        f.template singular<ratio<1> >(),
                        f.template plural<ratio<1> >(),
                        f.template short_name<ratio<1> >()
                    };
                    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    const std::basic_string<CharT>* k = chrono_detail::scan_keyword(i, e,
                                  units, units + sizeof(units)/sizeof(units[0]),
                                  //~ std::use_facet<std::ctype<CharT> >(loc),
                                  err);
                    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    is.setstate(err);
                    switch ((k - units) / 3)
                    {
                    case 0:
                      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                        break;
                    default:
                        is.setstate(err);
                        //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                        return is;
                    }
                }
                else
                {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    // parse SI name, short or long
                    const std::basic_string<CharT> units[] =
                    {
                        f.template singular<atto>(),
                        f.template plural<atto>(),
                        f.template short_name<atto>(),
                        f.template singular<femto>(),
                        f.template plural<femto>(),
                        f.template short_name<femto>(),
                        f.template singular<pico>(),
                        f.template plural<pico>(),
                        f.template short_name<pico>(),
                        f.template singular<nano>(),
                        f.template plural<nano>(),
                        f.template short_name<nano>(),
                        f.template singular<micro>(),
                        f.template plural<micro>(),
                        f.template short_name<micro>(),
                        f.template singular<milli>(),
                        f.template plural<milli>(),
                        f.template short_name<milli>(),
                        f.template singular<centi>(),
                        f.template plural<centi>(),
                        f.template short_name<centi>(),
                        f.template singular<deci>(),
                        f.template plural<deci>(),
                        f.template short_name<deci>(),
                        f.template singular<deca>(),
                        f.template plural<deca>(),
                        f.template short_name<deca>(),
                        f.template singular<hecto>(),
                        f.template plural<hecto>(),
                        f.template short_name<hecto>(),
                        f.template singular<kilo>(),
                        f.template plural<kilo>(),
                        f.template short_name<kilo>(),
                        f.template singular<mega>(),
                        f.template plural<mega>(),
                        f.template short_name<mega>(),
                        f.template singular<giga>(),
                        f.template plural<giga>(),
                        f.template short_name<giga>(),
                        f.template singular<tera>(),
                        f.template plural<tera>(),
                        f.template short_name<tera>(),
                        f.template singular<peta>(),
                        f.template plural<peta>(),
                        f.template short_name<peta>(),
                        f.template singular<exa>(),
                        f.template plural<exa>(),
                        f.template short_name<exa>(),
                        f.template singular<ratio<1> >(),
                        f.template plural<ratio<1> >(),
                        f.template short_name<ratio<1> >(),
                        f.template singular<ratio<60> >(),
                        f.template plural<ratio<60> >(),
                        f.template short_name<ratio<60> >(),
                        f.template singular<ratio<3600> >(),
                        f.template plural<ratio<3600> >(),
                        f.template short_name<ratio<3600> >()
                    };
                    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    const std::basic_string<CharT>* k = chrono_detail::scan_keyword(i, e,
                                  units, units + sizeof(units)/sizeof(units[0]),
                                  //~ std::use_facet<std::ctype<CharT> >(loc),
                                  err);
                    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    switch ((k - units) / 3)
                    {
                    case 0:
                        num = 1ULL;
                        den = 1000000000000000000ULL;
                        break;
                    case 1:
                        num = 1ULL;
                        den = 1000000000000000ULL;
                        break;
                    case 2:
                        num = 1ULL;
                        den = 1000000000000ULL;
                        break;
                    case 3:
                        num = 1ULL;
                        den = 1000000000ULL;
                        break;
                    case 4:
                        num = 1ULL;
                        den = 1000000ULL;
                        break;
                    case 5:
                        num = 1ULL;
                        den = 1000ULL;
                        break;
                    case 6:
                        num = 1ULL;
                        den = 100ULL;
                        break;
                    case 7:
                        num = 1ULL;
                        den = 10ULL;
                        break;
                    case 8:
                        num = 10ULL;
                        den = 1ULL;
                        break;
                    case 9:
                        num = 100ULL;
                        den = 1ULL;
                        break;
                    case 10:
                        num = 1000ULL;
                        den = 1ULL;
                        break;
                    case 11:
                        num = 1000000ULL;
                        den = 1ULL;
                        break;
                    case 12:
                        num = 1000000000ULL;
                        den = 1ULL;
                        break;
                    case 13:
                        num = 1000000000000ULL;
                        den = 1ULL;
                        break;
                    case 14:
                        num = 1000000000000000ULL;
                        den = 1ULL;
                        break;
                    case 15:
                        num = 1000000000000000000ULL;
                        den = 1ULL;
                        break;
                    case 16:
                        num = 1;
                        den = 1;
                        break;
                    case 17:
                        num = 60;
                        den = 1;
                        break;
                    case 18:
                        num = 3600;
                        den = 1;
                        break;
                    default:
                      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                        is.setstate(err|is.failbit);
                        return is;
                    }
                }
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                // unit is num/den
                // r should be multiplied by (num/den) / Period
                // Reduce (num/den) / Period to lowest terms
                unsigned long long gcd_n1_n2 = integer::gcd<unsigned long long>(num, Period::num);
                unsigned long long gcd_d1_d2 = integer::gcd<unsigned long long>(den, Period::den);
                num /= gcd_n1_n2;
                den /= gcd_d1_d2;
                unsigned long long n2 = Period::num / gcd_n1_n2;
                unsigned long long d2 = Period::den / gcd_d1_d2;
                if (num > (std::numeric_limits<unsigned long long>::max)() / d2 ||
                    den > (std::numeric_limits<unsigned long long>::max)() / n2)
                {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    // (num/den) / Period overflows
                    is.setstate(err|is.failbit);
                    return is;
                }
                num *= d2;
                den *= n2;

                typedef typename common_type<intermediate_type, unsigned long long>::type common_type_t;

                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                // num / den is now factor to multiply by r
                if (!chrono_detail::reduce(r, den, err))
                {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                  is.setstate(err|is.failbit);
                  return is;
                }

                //if (r > ((duration_values<common_type_t>::max)() / num))
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                if (chrono::detail::gt(r,((duration_values<common_type_t>::max)() / num)))
                {
                    // Conversion to Period overflowed
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    is.setstate(err|is.failbit);
                    return is;
                }
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                common_type_t t = r * num;
                t /= den;
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;

                if (t > duration_values<common_type_t>::zero())
                {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                  if ( (duration_values<Rep>::max)() < Rep(t))
                  {
                    //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                    // Conversion to Period overflowed
                    is.setstate(err|is.failbit);
                    return is;
                  }
                }
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                // Success!  Store it.
                d = duration<Rep, Period>(Rep(t));
                is.setstate(err);
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                return is;
            }
            else {
              //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                is.setstate(is.failbit | is.eofbit);
                return is;
            }
        }
        else
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
            if (i == e)
              is.setstate(is.failbit|is.eofbit);
            else
              is.setstate(is.failbit);
            //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
            return is;
        }
    }
    else {
      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        //is.setstate(is.failbit);
      return is;
    }
}


template <class CharT, class Traits, class Clock, class Duration>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const time_point<Clock, Duration>& tp)
{
    return os << tp.time_since_epoch() << clock_string<Clock, CharT>::since();
}

template <class CharT, class Traits, class Clock, class Duration>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is,
           time_point<Clock, Duration>& tp)
{
    Duration d;
    is >> d;
    if (is.good())
    {
        const std::basic_string<CharT> units=clock_string<Clock, CharT>::since();
        std::ios_base::iostate err = std::ios_base::goodbit;
        typedef std::istreambuf_iterator<CharT, Traits> in_iterator;
        in_iterator i(is);
        in_iterator e;
        std::ptrdiff_t k = chrono_detail::scan_keyword(i, e,
                      &units, &units + 1,
                      //~ std::use_facet<std::ctype<CharT> >(is.getloc()),
                      err) - &units;
        is.setstate(err);
        if (k == 1)
        {
          is.setstate(err | is.failbit);
            // failed to read epoch string
            return is;
        }
        tp = time_point<Clock, Duration>(d);
    }
    else
        is.setstate(is.failbit);
    return is;
}
}  // chrono

}

#endif  // BOOST_CHRONO_CHRONO_IO_HPP
