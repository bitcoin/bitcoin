#include <bson.h>
#include <fcntl.h>
#include <stdio.h>

#include "bson-tests.h"
#include "bson-iso8601-private.h"
#include "TestSuite.h"

#ifndef BINARY_DIR
#define BINARY_DIR "tests/binary"
#endif

#define IS_TIME_T_SMALL (sizeof (time_t) == sizeof (int32_t))

static void
test_date (const char *str, int64_t millis)
{
   int64_t v;
   bson_error_t error;

   if (!_bson_iso8601_date_parse (str, strlen (str), &v, &error)) {
      fprintf (stderr, "could not parse (%s)\n", str);
      abort ();
   }

   if (v != millis) {
      fprintf (stderr,
               "parsed value not correct: %" PRId64 " != %" PRId64 "\n",
               millis,
               v);

      fprintf (stderr, "parsing: [%s]\n", str);
      abort ();
   }
}


static void
test_date_io (const char *str_in, const char *str_out, int64_t millis)
{
   bson_string_t *bson_str;

   test_date (str_in, millis);

   bson_str = bson_string_new (NULL);
   _bson_iso8601_date_format (millis, bson_str);
   ASSERT_CMPSTR (bson_str->str, str_out);
   bson_string_free (bson_str, true);
}


/* round trip str -> millis -> str */
static void
test_date_rt (const char *str, int64_t millis)
{
   test_date_io (str, str, millis);
}


static void
test_date_should_fail (const char *str)
{
   int64_t v;
   bson_error_t error;

   if (_bson_iso8601_date_parse (str, strlen (str), &v, &error)) {
      fprintf (stderr, "should not be able to parse (%s)\n", str);
      abort ();
   }
}

static void
test_bson_iso8601_utc (void)
{
   /* Allowed date format:
    * YYYY-MM-DDTHH:MM[:SS[.m[m[m]]]]Z
    * Year, month, day, hour, and minute are required, while the seconds
    * component and one to
    * three milliseconds are optional.
    */

   test_date_rt ("1971-02-03T04:05:06.789Z", 34401906789ULL);
   test_date_io (
      "1971-02-03T04:05:06.78Z", "1971-02-03T04:05:06.780Z", 34401906780ULL);
   test_date_io (
      "1971-02-03T04:05:06.7Z", "1971-02-03T04:05:06.700Z", 34401906700ULL);
   test_date_rt ("1971-02-03T04:05:06Z", 34401906000ULL);
   test_date_io ("1971-02-03T04:05Z", "1971-02-03T04:05:00Z", 34401900000ULL);
   test_date_io ("1970-01-01T00:00:00.000Z", "1970-01-01T00:00:00Z", 0ULL);
   test_date_rt ("1970-06-30T01:06:40.981Z", 15556000981ULL);
   test_date ("1970-01-01T00:00:00.000+0100", -3600LL * 1000);

   if (!IS_TIME_T_SMALL) {
      test_date_rt ("2058-02-20T18:29:11.100Z", 2781455351100ULL);
      test_date ("3001-01-01T08:00:00.000Z", 32535244800000ULL);
   }

   test_date_rt ("2013-02-20T18:29:11.100Z", 1361384951100ULL);

   /* from the BSON Corpus Tests */
   test_date_io ("1970-01-01T00:00:00.000Z", "1970-01-01T00:00:00Z", 0);
   test_date_rt ("2012-12-24T12:15:30.500Z", 1356351330500);
   test_date ("1960-12-24T12:15:30.500Z", -284643869500LL);
}

static void
test_bson_iso8601_local (void)
{
   /* Allowed date format:
    * YYYY-MM-DDTHH:MM[:SS[.m[m[m]]]]+HHMM
    * Year, month, day, hour, and minute are required, while the seconds
    * component and one to
    * three milliseconds are optional.  The time zone offset must be four
    * digits.
    */

   test_date_io ("1971-02-03T09:16:06.789+0511",
                 "1971-02-03T04:05:06.789Z",
                 34401906789ULL);
   test_date_io ("1971-02-03T09:16:06.78+0511",
                 "1971-02-03T04:05:06.780Z",
                 34401906780ULL);
   test_date_io (
      "1971-02-03T09:16:06.7+0511", "1971-02-03T04:05:06.700Z", 34401906700ULL);
   test_date_io (
      "1971-02-03T09:16:06+0511", "1971-02-03T04:05:06Z", 34401906000ULL);
   test_date_io (
      "1971-02-03T09:16+0511", "1971-02-03T04:05:00Z", 34401900000ULL);
   test_date_io ("1970-01-01T00:00:00.000Z", "1970-01-01T00:00:00Z", 0ULL);
   test_date_rt ("1970-06-30T01:06:40.981Z", 15556000981ULL);
   test_date_io ("1970-06-29T21:06:40.981-0400",
                 "1970-06-30T01:06:40.981Z",
                 15556000981ULL);
   test_date_io ("1969-12-31T16:00:00.000-0800", "1970-01-01T00:00:00Z", 0ULL);

   if (!IS_TIME_T_SMALL) {
      test_date_io ("2058-02-20T13:29:11.100-0500",
                    "2058-02-20T18:29:11.100Z",
                    2781455351100ULL);
      test_date_rt ("3000-12-31T23:59:59Z", 32535215999000ULL);
   } else {
      test_date_rt ("2038-01-19T03:14:07Z", 2147483647000ULL);
   }

   test_date_io ("2013-02-20T13:29:11.100-0500",
                 "2013-02-20T18:29:11.100Z",
                 1361384951100ULL);
   test_date_io ("2013-02-20T13:29:11.100-0501",
                 "2013-02-20T18:30:11.100Z",
                 1361385011100ULL);
   test_date ("0000-01-01T00:00:00.000Z", -62167219200000LL);
   test_date ("0000-01-01T00:00:00.000+2300", -62167302000000LL);
   test_date ("9999-01-01T00:00:00.000Z", 253370764800000ULL);
}

static void
test_bson_iso8601_invalid (void)
{
   /* Invalid decimal */
   test_date_should_fail ("1970-01-01T00:00:00.0.0Z");
   test_date_should_fail ("1970-01-01T00:00:.0.000Z");
   test_date_should_fail ("1970-01-01T00:.0:00.000Z");
   test_date_should_fail ("1970-01-01T.0:00:00.000Z");
   test_date_should_fail ("1970-01-.1T00:00:00.000Z");
   test_date_should_fail ("1970-.1-01T00:00:00.000Z");
   test_date_should_fail (".970-01-01T00:00:00.000Z");

   /* Extra sign characters */
   test_date_should_fail ("1970-01-01T00:00:00.+00Z");
   test_date_should_fail ("1970-01-01T00:00:+0.000Z");
   test_date_should_fail ("1970-01-01T00:+0:00.000Z");
   test_date_should_fail ("1970-01-01T+0:00:00.000Z");
   test_date_should_fail ("1970-01-+1T00:00:00.000Z");
   test_date_should_fail ("1970-+1-01T00:00:00.000Z");
   test_date_should_fail ("+970-01-01T00:00:00.000Z");

   test_date_should_fail ("1970-01-01T00:00:00.-00Z");
   test_date_should_fail ("1970-01-01T00:00:-0.000Z");
   test_date_should_fail ("1970-01-01T00:-0:00.000Z");
   test_date_should_fail ("1970-01-01T-0:00:00.000Z");
   test_date_should_fail ("1970-01--1T00:00:00.000Z");
   test_date_should_fail ("1970--1-01T00:00:00.000Z");
   test_date_should_fail ("-970-01-01T00:00:00.000Z");

   /* Out of range */
   test_date_should_fail ("1970-01-01T00:60:00.000Z");
   test_date_should_fail ("1970-01-01T24:00:00.000Z");
   test_date_should_fail ("1970-01-32T00:00:00.000Z");
   test_date_should_fail ("1970-01-00T00:00:00.000Z");
   test_date_should_fail ("1970-13-01T00:00:00.000Z");
   test_date_should_fail ("1970-00-01T00:00:00.000Z");
   test_date_should_fail ("10000-01-01T00:00:00.000Z");
   test_date_should_fail ("-10000-01-01T00:00:00.000Z");

   /* Invalid lengths */
   test_date_should_fail ("01970-01-01T00:00:00.000Z");
   test_date_should_fail ("1970-001-01T00:00:00.000Z");
   test_date_should_fail ("1970-01-001T00:00:00.000Z");
   test_date_should_fail ("1970-01-01T000:00:00.000Z");
   test_date_should_fail ("1970-01-01T00:000:00.000Z");
   test_date_should_fail ("1970-01-01T00:00:000.000Z");
   test_date_should_fail ("1970-01-01T00:00:00.0000Z");
   test_date_should_fail ("197-01-01T00:00:00.000Z");
   test_date_should_fail ("1970-1-01T00:00:00.000Z");
   test_date_should_fail ("1970-01-1T00:00:00.000Z");
   test_date_should_fail ("1970-01-01T0:00:00.000Z");
   test_date_should_fail ("1970-01-01T00:0:00.000Z");
   test_date_should_fail ("1970-01-01T00:00:0.000Z");

   /* Invalid delimiters */
   test_date_should_fail ("1970+01-01T00:00:00.000Z");
   test_date_should_fail ("1970-01+01T00:00:00.000Z");
   test_date_should_fail ("1970-01-01Q00:00:00.000Z");
   test_date_should_fail ("1970-01-01T00-00:00.000Z");
   test_date_should_fail ("1970-01-01T00:00-00.000Z");
   test_date_should_fail ("1970-01-01T00:00:00-000Z");

   /* Missing numbers */
   test_date_should_fail ("1970--01T00:00:00.000Z");
   test_date_should_fail ("1970-01-T00:00:00.000Z");
   test_date_should_fail ("1970-01-01T:00:00.000Z");
   test_date_should_fail ("1970-01-01T00::00.000Z");
   test_date_should_fail ("1970-01-01T00:00:.000Z");
   test_date_should_fail ("1970-01-01T00:00:00.Z");

   /* Bad time offset field */
   test_date_should_fail ("1970-01-01T05:00:01ZZ");
   test_date_should_fail ("1970-01-01T05:00:01+");
   test_date_should_fail ("1970-01-01T05:00:01-");
   test_date_should_fail ("1970-01-01T05:00:01-11111");
   test_date_should_fail ("1970-01-01T05:00:01Z1111");
   test_date_should_fail ("1970-01-01T05:00:01+111");
   test_date_should_fail ("1970-01-01T05:00:01+1160");
   test_date_should_fail ("1970-01-01T05:00:01+2400");
   test_date_should_fail ("1970-01-01T05:00:01+00+0");

   /* Bad prefixes */
   test_date_should_fail ("1970-01-01T05:00:01.");
   test_date_should_fail ("1970-01-01T05:00:");
   test_date_should_fail ("1970-01-01T05:");
   test_date_should_fail ("1970-01-01T");
   test_date_should_fail ("1970-01-");
   test_date_should_fail ("1970-");
   test_date_should_fail ("1970-01-01T05+0500");
   test_date_should_fail ("1970-01-01+0500");
   test_date_should_fail ("1970-01+0500");
   test_date_should_fail ("1970+0500");
   test_date_should_fail ("1970-01-01T01Z");
   test_date_should_fail ("1970-01-01Z");
   test_date_should_fail ("1970-01Z");
   test_date_should_fail ("1970Z");

   /* No local time */
   test_date_should_fail ("1970-01-01T00:00:00.000");
   test_date_should_fail ("1970-01-01T00:00:00");
   test_date_should_fail ("1970-01-01T00:00");
   test_date_should_fail ("1970-01-01T00");
   test_date_should_fail ("1970-01-01");
   test_date_should_fail ("1970-01");
   test_date_should_fail ("1970");

   /* Invalid hex base specifiers */
   test_date_should_fail ("x970-01-01T00:00:00.000Z");
   test_date_should_fail ("1970-x1-01T00:00:00.000Z");
   test_date_should_fail ("1970-01-x1T00:00:00.000Z");
   test_date_should_fail ("1970-01-01Tx0:00:00.000Z");
   test_date_should_fail ("1970-01-01T00:x0:00.000Z");
   test_date_should_fail ("1970-01-01T00:00:x0.000Z");
   test_date_should_fail ("1970-01-01T00:00:00.x00Z");
}

static void
test_bson_iso8601_leap_year (void)
{
   test_date_rt ("1972-02-29T00:00:00Z", 68169600000ULL);
   test_date_rt ("1976-02-29T00:00:00Z", 194400000000ULL);
   test_date_rt ("1980-02-29T00:00:00Z", 320630400000ULL);
   test_date_rt ("1984-02-29T00:00:00Z", 446860800000ULL);
   test_date_rt ("1988-02-29T00:00:00Z", 573091200000ULL);
   test_date_rt ("1992-02-29T00:00:00Z", 699321600000ULL);
   test_date_rt ("1996-02-29T00:00:00Z", 825552000000ULL);
   test_date_rt ("2000-02-29T00:00:00Z", 951782400000ULL);
   test_date_rt ("2004-02-29T00:00:00Z", 1078012800000ULL);
   test_date_rt ("2008-02-29T00:00:00Z", 1204243200000ULL);
   test_date_rt ("2012-02-29T00:00:00Z", 1330473600000ULL);
   test_date_rt ("2016-02-29T00:00:00Z", 1456704000000ULL);
   test_date_rt ("2020-02-29T00:00:00Z", 1582934400000ULL);
   test_date_rt ("2024-02-29T00:00:00Z", 1709164800000ULL);
   test_date_rt ("2028-02-29T00:00:00Z", 1835395200000ULL);
   test_date_rt ("2032-02-29T00:00:00Z", 1961625600000ULL);
   test_date_rt ("2036-02-29T00:00:00Z", 2087856000000ULL);

   if (!IS_TIME_T_SMALL) {
      test_date_rt ("2040-02-29T00:00:00Z", 2214086400000ULL);
      test_date_rt ("2044-02-29T00:00:00Z", 2340316800000ULL);
      test_date_rt ("2048-02-29T00:00:00Z", 2466547200000ULL);
      test_date_rt ("2052-02-29T00:00:00Z", 2592777600000ULL);
      test_date_rt ("2056-02-29T00:00:00Z", 2719008000000ULL);
      test_date_rt ("2060-02-29T00:00:00Z", 2845238400000ULL);
      test_date_rt ("2064-02-29T00:00:00Z", 2971468800000ULL);
      test_date_rt ("2068-02-29T00:00:00Z", 3097699200000ULL);
      test_date_rt ("2072-02-29T00:00:00Z", 3223929600000ULL);
      test_date_rt ("2076-02-29T00:00:00Z", 3350160000000ULL);
      test_date_rt ("2080-02-29T00:00:00Z", 3476390400000ULL);
      test_date_rt ("2084-02-29T00:00:00Z", 3602620800000ULL);
      test_date_rt ("2088-02-29T00:00:00Z", 3728851200000ULL);
      test_date_rt ("2092-02-29T00:00:00Z", 3855081600000ULL);
      test_date_rt ("2096-02-29T00:00:00Z", 3981312000000ULL);
      test_date_rt ("2104-02-29T00:00:00Z", 4233686400000ULL);
      test_date_rt ("2108-02-29T00:00:00Z", 4359916800000ULL);
      test_date_rt ("2112-02-29T00:00:00Z", 4486147200000ULL);
      test_date_rt ("2116-02-29T00:00:00Z", 4612377600000ULL);
      test_date_rt ("2120-02-29T00:00:00Z", 4738608000000ULL);
      test_date_rt ("2124-02-29T00:00:00Z", 4864838400000ULL);
      test_date_rt ("2128-02-29T00:00:00Z", 4991068800000ULL);
      test_date_rt ("2132-02-29T00:00:00Z", 5117299200000ULL);
      test_date_rt ("2136-02-29T00:00:00Z", 5243529600000ULL);
      test_date_rt ("2140-02-29T00:00:00Z", 5369760000000ULL);
      test_date_rt ("2144-02-29T00:00:00Z", 5495990400000ULL);
      test_date_rt ("2148-02-29T00:00:00Z", 5622220800000ULL);
      test_date_rt ("2152-02-29T00:00:00Z", 5748451200000ULL);
      test_date_rt ("2156-02-29T00:00:00Z", 5874681600000ULL);
      test_date_rt ("2160-02-29T00:00:00Z", 6000912000000ULL);
      test_date_rt ("2164-02-29T00:00:00Z", 6127142400000ULL);
      test_date_rt ("2168-02-29T00:00:00Z", 6253372800000ULL);
      test_date_rt ("2172-02-29T00:00:00Z", 6379603200000ULL);
      test_date_rt ("2176-02-29T00:00:00Z", 6505833600000ULL);
      test_date_rt ("2180-02-29T00:00:00Z", 6632064000000ULL);
      test_date_rt ("2184-02-29T00:00:00Z", 6758294400000ULL);
      test_date_rt ("2188-02-29T00:00:00Z", 6884524800000ULL);
      test_date_rt ("2192-02-29T00:00:00Z", 7010755200000ULL);
      test_date_rt ("2196-02-29T00:00:00Z", 7136985600000ULL);
      test_date_rt ("2204-02-29T00:00:00Z", 7389360000000ULL);
      test_date_rt ("2208-02-29T00:00:00Z", 7515590400000ULL);
      test_date_rt ("2212-02-29T00:00:00Z", 7641820800000ULL);
      test_date_rt ("2216-02-29T00:00:00Z", 7768051200000ULL);
      test_date_rt ("2220-02-29T00:00:00Z", 7894281600000ULL);
      test_date_rt ("2224-02-29T00:00:00Z", 8020512000000ULL);
      test_date_rt ("2228-02-29T00:00:00Z", 8146742400000ULL);
      test_date_rt ("2232-02-29T00:00:00Z", 8272972800000ULL);
      test_date_rt ("2236-02-29T00:00:00Z", 8399203200000ULL);
      test_date_rt ("2240-02-29T00:00:00Z", 8525433600000ULL);
      test_date_rt ("2244-02-29T00:00:00Z", 8651664000000ULL);
      test_date_rt ("2248-02-29T00:00:00Z", 8777894400000ULL);
      test_date_rt ("2252-02-29T00:00:00Z", 8904124800000ULL);
      test_date_rt ("2256-02-29T00:00:00Z", 9030355200000ULL);
      test_date_rt ("2260-02-29T00:00:00Z", 9156585600000ULL);
      test_date_rt ("2264-02-29T00:00:00Z", 9282816000000ULL);
      test_date_rt ("2268-02-29T00:00:00Z", 9409046400000ULL);
      test_date_rt ("2272-02-29T00:00:00Z", 9535276800000ULL);
      test_date_rt ("2276-02-29T00:00:00Z", 9661507200000ULL);
      test_date_rt ("2280-02-29T00:00:00Z", 9787737600000ULL);
      test_date_rt ("2284-02-29T00:00:00Z", 9913968000000ULL);
      test_date_rt ("2288-02-29T00:00:00Z", 10040198400000ULL);
      test_date_rt ("2292-02-29T00:00:00Z", 10166428800000ULL);
      test_date_rt ("2296-02-29T00:00:00Z", 10292659200000ULL);
      test_date_rt ("2304-02-29T00:00:00Z", 10545033600000ULL);
      test_date_rt ("2308-02-29T00:00:00Z", 10671264000000ULL);
      test_date_rt ("2312-02-29T00:00:00Z", 10797494400000ULL);
      test_date_rt ("2316-02-29T00:00:00Z", 10923724800000ULL);
      test_date_rt ("2320-02-29T00:00:00Z", 11049955200000ULL);
      test_date_rt ("2324-02-29T00:00:00Z", 11176185600000ULL);
      test_date_rt ("2328-02-29T00:00:00Z", 11302416000000ULL);
      test_date_rt ("2332-02-29T00:00:00Z", 11428646400000ULL);
      test_date_rt ("2336-02-29T00:00:00Z", 11554876800000ULL);
      test_date_rt ("2340-02-29T00:00:00Z", 11681107200000ULL);
      test_date_rt ("2344-02-29T00:00:00Z", 11807337600000ULL);
      test_date_rt ("2348-02-29T00:00:00Z", 11933568000000ULL);
      test_date_rt ("2352-02-29T00:00:00Z", 12059798400000ULL);
      test_date_rt ("2356-02-29T00:00:00Z", 12186028800000ULL);
      test_date_rt ("2360-02-29T00:00:00Z", 12312259200000ULL);
      test_date_rt ("2364-02-29T00:00:00Z", 12438489600000ULL);
      test_date_rt ("2368-02-29T00:00:00Z", 12564720000000ULL);
      test_date_rt ("2372-02-29T00:00:00Z", 12690950400000ULL);
      test_date_rt ("2376-02-29T00:00:00Z", 12817180800000ULL);
      test_date_rt ("2380-02-29T00:00:00Z", 12943411200000ULL);
      test_date_rt ("2384-02-29T00:00:00Z", 13069641600000ULL);
      test_date_rt ("2388-02-29T00:00:00Z", 13195872000000ULL);
      test_date_rt ("2392-02-29T00:00:00Z", 13322102400000ULL);
      test_date_rt ("2396-02-29T00:00:00Z", 13448332800000ULL);
      test_date_rt ("2400-02-29T00:00:00Z", 13574563200000ULL);
   }
}

void
test_iso8601_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/iso8601/utc", test_bson_iso8601_utc);
   TestSuite_Add (suite, "/bson/iso8601/local", test_bson_iso8601_local);
   TestSuite_Add (suite, "/bson/iso8601/invalid", test_bson_iso8601_invalid);
   TestSuite_Add (
      suite, "/bson/iso8601/leap_year", test_bson_iso8601_leap_year);
}
