/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/
#include <boost/predef/detail/test.h>

#ifndef BOOST_PREDEF_MAKE_H
#define BOOST_PREDEF_MAKE_H

/*
Shorthands for the common version number formats used by vendors...
*/

/* tag::reference[]
= `BOOST_PREDEF_MAKE_..` macros

These set of macros decompose common vendor version number
macros which are composed version, revision, and patch digits.
The naming convention indicates:

* The base of the specified version number. "`BOOST_PREDEF_MAKE_0X`" for
  hexadecimal digits, and "`BOOST_PREDEF_MAKE_10`" for decimal digits.
* The format of the vendor version number. Where "`V`" indicates the version digits,
  "`R`" indicates the revision digits, "`P`" indicates the patch digits, and "`0`"
  indicates an ignored digit.

Macros are:

*/ // end::reference[]
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VRP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VRP(V) BOOST_VERSION_NUMBER((V&0xF00)>>8,(V&0xF0)>>4,(V&0xF))
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VVRP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VVRP(V) BOOST_VERSION_NUMBER((V&0xFF00)>>8,(V&0xF0)>>4,(V&0xF))
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VRPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VRPP(V) BOOST_VERSION_NUMBER((V&0xF000)>>12,(V&0xF00)>>8,(V&0xFF))
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VVRR(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VVRR(V) BOOST_VERSION_NUMBER((V&0xFF00)>>8,(V&0xFF),0)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VRRPPPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VRRPPPP(V) BOOST_VERSION_NUMBER((V&0xF000000)>>24,(V&0xFF0000)>>16,(V&0xFFFF))
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VVRRP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VVRRP(V) BOOST_VERSION_NUMBER((V&0xFF000)>>12,(V&0xFF0)>>4,(V&0xF))
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VRRPP000(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VRRPP000(V) BOOST_VERSION_NUMBER((V&0xF0000000)>>28,(V&0xFF00000)>>20,(V&0xFF000)>>12)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_0X_VVRRPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_0X_VVRRPP(V) BOOST_VERSION_NUMBER((V&0xFF0000)>>16,(V&0xFF00)>>8,(V&0xFF))
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VPPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VPPP(V) BOOST_VERSION_NUMBER(((V)/1000)%10,0,(V)%1000)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVPPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVPPP(V) BOOST_VERSION_NUMBER(((V)/1000)%100,0,(V)%1000)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VR0(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VR0(V) BOOST_VERSION_NUMBER(((V)/100)%10,((V)/10)%10,0)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VRP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VRP(V) BOOST_VERSION_NUMBER(((V)/100)%10,((V)/10)%10,(V)%10)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VRP000(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VRP000(V) BOOST_VERSION_NUMBER(((V)/100000)%10,((V)/10000)%10,((V)/1000)%10)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VRPPPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VRPPPP(V) BOOST_VERSION_NUMBER(((V)/100000)%10,((V)/10000)%10,(V)%10000)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VRPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VRPP(V) BOOST_VERSION_NUMBER(((V)/1000)%10,((V)/100)%10,(V)%100)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VRR(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VRR(V) BOOST_VERSION_NUMBER(((V)/100)%10,(V)%100,0)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VRRPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VRRPP(V) BOOST_VERSION_NUMBER(((V)/10000)%10,((V)/100)%100,(V)%100)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VRR000(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VRR000(V) BOOST_VERSION_NUMBER(((V)/100000)%10,((V)/1000)%100,0)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VV00(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VV00(V) BOOST_VERSION_NUMBER(((V)/100)%100,0,0)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVRR(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVRR(V) BOOST_VERSION_NUMBER(((V)/100)%100,(V)%100,0)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVRRP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVRRP(V) BOOST_VERSION_NUMBER(((V)/1000)%100,((V)/10)%100,(V)%10)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVRRPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVRRPP(V) BOOST_VERSION_NUMBER(((V)/10000)%100,((V)/100)%100,(V)%100)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVRRPPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVRRPPP(V) BOOST_VERSION_NUMBER(((V)/100000)%100,((V)/1000)%100,(V)%1000)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVRR0PP00(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVRR0PP00(V) BOOST_VERSION_NUMBER(((V)/10000000)%100,((V)/100000)%100,((V)/100)%100)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVRR0PPPP(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVRR0PPPP(V) BOOST_VERSION_NUMBER(((V)/10000000)%100,((V)/100000)%100,(V)%10000)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_10_VVRR00PP00(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_10_VVRR00PP00(V) BOOST_VERSION_NUMBER(((V)/100000000)%100,((V)/1000000)%100,((V)/100)%100)

/* tag::reference[]

= `BOOST_PREDEF_MAKE_*..` date macros

Date decomposition macros return a date in the relative to the 1970
Epoch date. If the month is not available, January 1st is used as the month and day.
If the day is not available, but the month is, the 1st of the month is used as the day.

*/ // end::reference[]
/* tag::reference[]
* `BOOST_PREDEF_MAKE_DATE(Y,M,D)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_DATE(Y,M,D) BOOST_VERSION_NUMBER((Y)%10000-1970,(M)%100,(D)%100)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_YYYYMMDD(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_YYYYMMDD(V) BOOST_PREDEF_MAKE_DATE(((V)/10000)%10000,((V)/100)%100,(V)%100)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_YYYY(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_YYYY(V) BOOST_PREDEF_MAKE_DATE(V,1,1)
/* tag::reference[]
* `BOOST_PREDEF_MAKE_YYYYMM(V)`
*/ // end::reference[]
#define BOOST_PREDEF_MAKE_YYYYMM(V) BOOST_PREDEF_MAKE_DATE((V)/100,(V)%100,1)

#endif
