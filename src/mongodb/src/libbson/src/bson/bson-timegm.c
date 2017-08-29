/*
** The original version of this file is in the public domain, so clarified as of
** 1996-06-05 by Arthur David Olson.
*/

/*
** Leap second handling from Bradley White.
** POSIX-style TZ environment variable handling from Guy Harris.
** Updated to use int64_t's instead of system-dependent definitions of int64_t
** and struct tm by A. Jesse Jiryu Davis for MongoDB, Inc.
*/

#include "bson-compat.h"
#include "bson-macros.h"
#include "bson-timegm-private.h"

#include "errno.h"
#include "string.h"
#include <stdint.h> /* for INT64_MAX and INT64_MIN */

/* Unlike <ctype.h>'s isdigit, this also works if c < 0 | c > UCHAR_MAX. */
#define is_digit(c) ((unsigned) (c) - '0' <= 9)

#if 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#define ATTRIBUTE_CONST __attribute__ ((const))
#define ATTRIBUTE_PURE __attribute__ ((__pure__))
#define ATTRIBUTE_FORMAT(spec) __attribute__ ((__format__ spec))
#else
#define ATTRIBUTE_CONST        /* empty */
#define ATTRIBUTE_PURE         /* empty */
#define ATTRIBUTE_FORMAT(spec) /* empty */
#endif

#if !defined _Noreturn && \
   (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112)
#if 2 < __GNUC__ + (8 <= __GNUC_MINOR__)
#define _Noreturn __attribute__((__noreturn__))
#else
#define _Noreturn
#endif
#endif

#if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901) && \
   !defined restrict
#define restrict /* empty */
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshift-negative-value"
#endif

/* The minimum and maximum finite time values.  */
static int64_t const time_t_min = INT64_MIN;
static int64_t const time_t_max = INT64_MAX;

#ifdef __clang__
#pragma clang diagnostic pop
#pragma clang diagnostic pop
#endif

#ifndef TZ_MAX_TIMES
#define TZ_MAX_TIMES 2000
#endif /* !defined TZ_MAX_TIMES */

#ifndef TZ_MAX_TYPES
/* This must be at least 17 for Europe/Samara and Europe/Vilnius.  */
#define TZ_MAX_TYPES 256 /* Limited by what (unsigned char)'s can hold */
#endif                   /* !defined TZ_MAX_TYPES */

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS 50 /* Maximum number of abbreviation characters */
                        /* (limited by what unsigned chars can hold) */
#endif                  /* !defined TZ_MAX_CHARS */

#ifndef TZ_MAX_LEAPS
#define TZ_MAX_LEAPS 50 /* Maximum number of leap second corrections */
#endif                  /* !defined TZ_MAX_LEAPS */

#define SECSPERMIN 60
#define MINSPERHOUR 60
#define HOURSPERDAY 24
#define DAYSPERWEEK 7
#define DAYSPERNYEAR 365
#define DAYSPERLYEAR 366
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY ((int_fast32_t) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR 12

#define TM_SUNDAY 0
#define TM_MONDAY 1
#define TM_TUESDAY 2
#define TM_WEDNESDAY 3
#define TM_THURSDAY 4
#define TM_FRIDAY 5
#define TM_SATURDAY 6

#define TM_JANUARY 0
#define TM_FEBRUARY 1
#define TM_MARCH 2
#define TM_APRIL 3
#define TM_MAY 4
#define TM_JUNE 5
#define TM_JULY 6
#define TM_AUGUST 7
#define TM_SEPTEMBER 8
#define TM_OCTOBER 9
#define TM_NOVEMBER 10
#define TM_DECEMBER 11

#define TM_YEAR_BASE 1900

#define EPOCH_YEAR 1970
#define EPOCH_WDAY TM_THURSDAY

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

/*
** Since everything in isleap is modulo 400 (or a factor of 400), we know that
**	isleap(y) == isleap(y % 400)
** and so
**	isleap(a + b) == isleap((a + b) % 400)
** or
**	isleap(a + b) == isleap(a % 400 + b % 400)
** This is true even if % means modulo rather than Fortran remainder
** (which is allowed by C89 but not C99).
** We use this to avoid addition overflow problems.
*/

#define isleap_sum(a, b) isleap ((a) % 400 + (b) % 400)

#ifndef TZ_ABBR_MAX_LEN
#define TZ_ABBR_MAX_LEN 16
#endif /* !defined TZ_ABBR_MAX_LEN */

#ifndef TZ_ABBR_CHAR_SET
#define TZ_ABBR_CHAR_SET \
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :+-._"
#endif /* !defined TZ_ABBR_CHAR_SET */

#ifndef TZ_ABBR_ERR_CHAR
#define TZ_ABBR_ERR_CHAR '_'
#endif /* !defined TZ_ABBR_ERR_CHAR */

#ifndef WILDABBR
/*
** Someone might make incorrect use of a time zone abbreviation:
**	1.	They might reference tzname[0] before calling tzset (explicitly
**		or implicitly).
**	2.	They might reference tzname[1] before calling tzset (explicitly
**		or implicitly).
**	3.	They might reference tzname[1] after setting to a time zone
**		in which Daylight Saving Time is never observed.
**	4.	They might reference tzname[0] after setting to a time zone
**		in which Standard Time is never observed.
**	5.	They might reference tm.TM_ZONE after calling offtime.
** What's best to do in the above cases is open to debate;
** for now, we just set things up so that in any of the five cases
** WILDABBR is used. Another possibility: initialize tzname[0] to the
** string "tzname[0] used before set", and similarly for the other cases.
** And another: initialize tzname[0] to "ERA", with an explanation in the
** manual page of what this "time zone abbreviation" means (doing this so
** that tzname[0] has the "normal" length of three characters).
*/
#define WILDABBR "   "
#endif /* !defined WILDABBR */

#ifdef TM_ZONE
static const char wildabbr[] = WILDABBR;
static const char gmt[] = "GMT";
#endif

struct ttinfo {            /* time type information */
   int_fast32_t tt_gmtoff; /* UT offset in seconds */
   int tt_isdst;           /* used to set tm_isdst */
   int tt_abbrind;         /* abbreviation list index */
   int tt_ttisstd;         /* true if transition is std time */
   int tt_ttisgmt;         /* true if transition is UT */
};

struct lsinfo {          /* leap second information */
   int64_t ls_trans;      /* transition time */
   int_fast64_t ls_corr; /* correction to apply */
};

#define BIGGEST(a, b) (((a) > (b)) ? (a) : (b))

#ifdef TZNAME_MAX
#define MY_TZNAME_MAX TZNAME_MAX
#endif /* defined TZNAME_MAX */
#ifndef TZNAME_MAX
#define MY_TZNAME_MAX 255
#endif /* !defined TZNAME_MAX */

struct state {
   int leapcnt;
   int timecnt;
   int typecnt;
   int charcnt;
   int goback;
   int goahead;
   int64_t ats[TZ_MAX_TIMES];
   unsigned char types[TZ_MAX_TIMES];
   struct ttinfo ttis[TZ_MAX_TYPES];
   char chars[BIGGEST (TZ_MAX_CHARS + 1, (2 * (MY_TZNAME_MAX + 1)))];
   struct lsinfo lsis[TZ_MAX_LEAPS];
   int defaulttype; /* for early times or if no transitions */
};

struct rule {
   int r_type;          /* type of rule--see below */
   int r_day;           /* day number of rule */
   int r_week;          /* week number of rule */
   int r_mon;           /* month number of rule */
   int_fast32_t r_time; /* transition time of rule */
};

#define JULIAN_DAY 0            /* Jn - Julian day */
#define DAY_OF_YEAR 1           /* n - day of year */
#define MONTH_NTH_DAY_OF_WEEK 2 /* Mm.n.d - month, week, day of week */

/*
** Prototypes for static functions.
*/

static void
gmtload (struct state *sp);
static struct bson_tm *
gmtsub (const int64_t *timep, int_fast32_t offset, struct bson_tm *tmp);
static int64_t
increment_overflow (int64_t *number, int64_t delta);
static int64_t
leaps_thru_end_of (int64_t y) ATTRIBUTE_PURE;
static int64_t
increment_overflow32 (int_fast32_t *number, int64_t delta);
static int64_t
normalize_overflow32 (int_fast32_t *tensptr, int64_t *unitsptr, int64_t base);
static int64_t
normalize_overflow (int64_t *tensptr, int64_t *unitsptr, int64_t base);
static int64_t
time1 (struct bson_tm *tmp,
       struct bson_tm *(*funcp) (const int64_t *, int_fast32_t, struct bson_tm *),
       int_fast32_t offset);
static int64_t
time2 (struct bson_tm *tmp,
       struct bson_tm *(*funcp) (const int64_t *, int_fast32_t, struct bson_tm *),
       int_fast32_t offset,
       int64_t *okayp);
static int64_t
time2sub (struct bson_tm *tmp,
          struct bson_tm *(*funcp) (const int64_t *, int_fast32_t, struct bson_tm *),
          int_fast32_t offset,
          int64_t *okayp,
          int64_t do_norm_secs);
static struct bson_tm *
timesub (const int64_t *timep,
         int_fast32_t offset,
         const struct state *sp,
         struct bson_tm *tmp);
static int64_t
tmcomp (const struct bson_tm *atmp, const struct bson_tm *btmp);

static struct state gmtmem;
#define gmtptr (&gmtmem)

static int gmt_is_set;

static const int mon_lengths[2][MONSPERYEAR] = {
   {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
   {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

static const int year_lengths[2] = {DAYSPERNYEAR, DAYSPERLYEAR};

static void
gmtload (struct state *const sp)
{
   memset (sp, 0, sizeof (struct state));
   sp->typecnt = 1;
   sp->charcnt = 4;
   sp->chars[0] = 'G';
   sp->chars[1] = 'M';
   sp->chars[2] = 'T';
}

/*
** gmtsub is to gmtime as localsub is to localtime.
*/

static struct bson_tm *
gmtsub (const int64_t *const timep,
        const int_fast32_t offset,
        struct bson_tm *const tmp)
{
   register struct bson_tm *result;

   if (!gmt_is_set) {
      gmt_is_set = true;
      gmtload (gmtptr);
   }
   result = timesub (timep, offset, gmtptr, tmp);
#ifdef TM_ZONE
   /*
   ** Could get fancy here and deliver something such as
   ** "UT+xxxx" or "UT-xxxx" if offset is non-zero,
   ** but this is no time for a treasure hunt.
   */
   tmp->TM_ZONE = offset ? wildabbr : gmtptr ? gmtptr->chars : gmt;
#endif /* defined TM_ZONE */
   return result;
}

/*
** Return the number of leap years through the end of the given year
** where, to make the math easy, the answer for year zero is defined as zero.
*/

static int64_t
leaps_thru_end_of (register const int64_t y)
{
   return (y >= 0) ? (y / 4 - y / 100 + y / 400)
                   : -(leaps_thru_end_of (-(y + 1)) + 1);
}

static struct bson_tm *
timesub (const int64_t *const timep,
         const int_fast32_t offset,
         register const struct state *const sp,
         register struct bson_tm *const tmp)
{
   register const struct lsinfo *lp;
   register int64_t tdays;
   register int64_t idays; /* unsigned would be so 2003 */
   register int_fast64_t rem;
   int64_t y;
   register const int *ip;
   register int_fast64_t corr;
   register int64_t hit;
   register int64_t i;

   corr = 0;
   hit = 0;
   i = (sp == NULL) ? 0 : sp->leapcnt;
   while (--i >= 0) {
      lp = &sp->lsis[i];
      if (*timep >= lp->ls_trans) {
         if (*timep == lp->ls_trans) {
            hit = ((i == 0 && lp->ls_corr > 0) ||
                   lp->ls_corr > sp->lsis[i - 1].ls_corr);
            if (hit)
               while (i > 0 &&
                      sp->lsis[i].ls_trans == sp->lsis[i - 1].ls_trans + 1 &&
                      sp->lsis[i].ls_corr == sp->lsis[i - 1].ls_corr + 1) {
                  ++hit;
                  --i;
               }
         }
         corr = lp->ls_corr;
         break;
      }
   }
   y = EPOCH_YEAR;
   tdays = *timep / SECSPERDAY;
   rem = *timep - tdays * SECSPERDAY;
   while (tdays < 0 || tdays >= year_lengths[isleap (y)]) {
      int64_t newy;
      register int64_t tdelta;
      register int64_t idelta;
      register int64_t leapdays;

      tdelta = tdays / DAYSPERLYEAR;
      idelta = tdelta;
      if (idelta == 0)
         idelta = (tdays < 0) ? -1 : 1;
      newy = y;
      if (increment_overflow (&newy, idelta))
         return NULL;
      leapdays = leaps_thru_end_of (newy - 1) - leaps_thru_end_of (y - 1);
      tdays -= ((int64_t) newy - y) * DAYSPERNYEAR;
      tdays -= leapdays;
      y = newy;
   }
   {
      register int_fast32_t seconds;

      seconds = (int_fast32_t) (tdays * SECSPERDAY);
      tdays = seconds / SECSPERDAY;
      rem += seconds - tdays * SECSPERDAY;
   }
   /*
   ** Given the range, we can now fearlessly cast...
   */
   idays = (int64_t) tdays;
   rem += offset - corr;
   while (rem < 0) {
      rem += SECSPERDAY;
      --idays;
   }
   while (rem >= SECSPERDAY) {
      rem -= SECSPERDAY;
      ++idays;
   }
   while (idays < 0) {
      if (increment_overflow (&y, -1))
         return NULL;
      idays += year_lengths[isleap (y)];
   }
   while (idays >= year_lengths[isleap (y)]) {
      idays -= year_lengths[isleap (y)];
      if (increment_overflow (&y, 1))
         return NULL;
   }
   tmp->tm_year = y;
   if (increment_overflow (&tmp->tm_year, -TM_YEAR_BASE))
      return NULL;
   tmp->tm_yday = idays;
   /*
   ** The "extra" mods below avoid overflow problems.
   */
   tmp->tm_wday =
      EPOCH_WDAY +
      ((y - EPOCH_YEAR) % DAYSPERWEEK) * (DAYSPERNYEAR % DAYSPERWEEK) +
      leaps_thru_end_of (y - 1) - leaps_thru_end_of (EPOCH_YEAR - 1) + idays;
   tmp->tm_wday %= DAYSPERWEEK;
   if (tmp->tm_wday < 0)
      tmp->tm_wday += DAYSPERWEEK;
   tmp->tm_hour = (int64_t) (rem / SECSPERHOUR);
   rem %= SECSPERHOUR;
   tmp->tm_min = (int64_t) (rem / SECSPERMIN);
   /*
   ** A positive leap second requires a special
   ** representation. This uses "... ??:59:60" et seq.
   */
   tmp->tm_sec = (int64_t) (rem % SECSPERMIN) + hit;
   ip = mon_lengths[isleap (y)];
   for (tmp->tm_mon = 0; idays >= ip[tmp->tm_mon]; ++(tmp->tm_mon))
      idays -= ip[tmp->tm_mon];
   tmp->tm_mday = (int64_t) (idays + 1);
   tmp->tm_isdst = 0;
#ifdef TM_GMTOFF
   tmp->TM_GMTOFF = offset;
#endif /* defined TM_GMTOFF */
   return tmp;
}

/*
** Adapted from code provided by Robert Elz, who writes:
**	The "best" way to do mktime I think is based on an idea of Bob
**	Kridle's (so its said...) from a long time ago.
**	It does a binary search of the int64_t space. Since int64_t's are
**	just 32 bits, its a max of 32 iterations (even at 64 bits it
**	would still be very reasonable).
*/

#ifndef WRONG
#define WRONG (-1)
#endif /* !defined WRONG */

/*
** Normalize logic courtesy Paul Eggert.
*/

static int64_t
increment_overflow (int64_t *const ip, int64_t j)
{
   register int64_t const i = *ip;

   /*
   ** If i >= 0 there can only be overflow if i + j > INT_MAX
   ** or if j > INT_MAX - i; given i >= 0, INT_MAX - i cannot overflow.
   ** If i < 0 there can only be overflow if i + j < INT_MIN
   ** or if j < INT_MIN - i; given i < 0, INT_MIN - i cannot overflow.
   */
   if ((i >= 0) ? (j > INT_MAX - i) : (j < INT_MIN - i))
      return true;
   *ip += j;
   return false;
}

static int64_t
increment_overflow32 (int_fast32_t *const lp, int64_t const m)
{
   register int_fast32_t const l = *lp;

   if ((l >= 0) ? (m > INT_FAST32_MAX - l) : (m < INT_FAST32_MIN - l))
      return true;
   *lp += m;
   return false;
}

static int64_t
normalize_overflow (int64_t *const tensptr, int64_t *const unitsptr, const int64_t base)
{
   register int64_t tensdelta;

   tensdelta =
      (*unitsptr >= 0) ? (*unitsptr / base) : (-1 - (-1 - *unitsptr) / base);
   *unitsptr -= tensdelta * base;
   return increment_overflow (tensptr, tensdelta);
}

static int64_t
normalize_overflow32 (int_fast32_t *const tensptr,
                      int64_t *const unitsptr,
                      const int64_t base)
{
   register int64_t tensdelta;

   tensdelta =
      (*unitsptr >= 0) ? (*unitsptr / base) : (-1 - (-1 - *unitsptr) / base);
   *unitsptr -= tensdelta * base;
   return increment_overflow32 (tensptr, tensdelta);
}

static int64_t
tmcomp (register const struct bson_tm *const atmp,
        register const struct bson_tm *const btmp)
{
   register int64_t result;

   if (atmp->tm_year != btmp->tm_year)
      return atmp->tm_year < btmp->tm_year ? -1 : 1;
   if ((result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
       (result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
       (result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
       (result = (atmp->tm_min - btmp->tm_min)) == 0)
      result = atmp->tm_sec - btmp->tm_sec;
   return result;
}

static int64_t
time2sub (struct bson_tm *const tmp,
          struct bson_tm *(*const funcp) (const int64_t *, int_fast32_t, struct bson_tm *),
          const int_fast32_t offset,
          int64_t *const okayp,
          const int64_t do_norm_secs)
{
   register const struct state *sp;
   register int64_t dir;
   register int64_t i, j;
   register int64_t saved_seconds;
   register int_fast32_t li;
   register int64_t lo;
   register int64_t hi;
   int_fast32_t y;
   int64_t newt;
   int64_t t;
   struct bson_tm yourtm, mytm;

   *okayp = false;
   yourtm = *tmp;
   if (do_norm_secs) {
      if (normalize_overflow (&yourtm.tm_min, &yourtm.tm_sec, SECSPERMIN))
         return WRONG;
   }
   if (normalize_overflow (&yourtm.tm_hour, &yourtm.tm_min, MINSPERHOUR))
      return WRONG;
   if (normalize_overflow (&yourtm.tm_mday, &yourtm.tm_hour, HOURSPERDAY))
      return WRONG;
   y = (int_fast32_t) yourtm.tm_year;
   if (normalize_overflow32 (&y, &yourtm.tm_mon, MONSPERYEAR))
      return WRONG;
   /*
   ** Turn y into an actual year number for now.
   ** It is converted back to an offset from TM_YEAR_BASE later.
   */
   if (increment_overflow32 (&y, TM_YEAR_BASE))
      return WRONG;
   while (yourtm.tm_mday <= 0) {
      if (increment_overflow32 (&y, -1))
         return WRONG;
      li = y + (1 < yourtm.tm_mon);
      yourtm.tm_mday += year_lengths[isleap (li)];
   }
   while (yourtm.tm_mday > DAYSPERLYEAR) {
      li = y + (1 < yourtm.tm_mon);
      yourtm.tm_mday -= year_lengths[isleap (li)];
      if (increment_overflow32 (&y, 1))
         return WRONG;
   }
   for (;;) {
      i = mon_lengths[isleap (y)][yourtm.tm_mon];
      if (yourtm.tm_mday <= i)
         break;
      yourtm.tm_mday -= i;
      if (++yourtm.tm_mon >= MONSPERYEAR) {
         yourtm.tm_mon = 0;
         if (increment_overflow32 (&y, 1))
            return WRONG;
      }
   }
   if (increment_overflow32 (&y, -TM_YEAR_BASE))
      return WRONG;
   yourtm.tm_year = y;
   if (yourtm.tm_year != y)
      return WRONG;
   if (yourtm.tm_sec >= 0 && yourtm.tm_sec < SECSPERMIN)
      saved_seconds = 0;
   else if (y + TM_YEAR_BASE < EPOCH_YEAR) {
      /*
      ** We can't set tm_sec to 0, because that might push the
      ** time below the minimum representable time.
      ** Set tm_sec to 59 instead.
      ** This assumes that the minimum representable time is
      ** not in the same minute that a leap second was deleted from,
      ** which is a safer assumption than using 58 would be.
      */
      if (increment_overflow (&yourtm.tm_sec, 1 - SECSPERMIN))
         return WRONG;
      saved_seconds = yourtm.tm_sec;
      yourtm.tm_sec = SECSPERMIN - 1;
   } else {
      saved_seconds = yourtm.tm_sec;
      yourtm.tm_sec = 0;
   }
   /*
   ** Do a binary search.
   */
   lo = INT64_MIN;
   hi = INT64_MAX;

   for (;;) {
      t = lo / 2 + hi / 2;
      if (t < lo)
         t = lo;
      else if (t > hi)
         t = hi;
      if ((*funcp) (&t, offset, &mytm) == NULL) {
         /*
         ** Assume that t is too extreme to be represented in
         ** a struct bson_tm; arrange things so that it is less
         ** extreme on the next pass.
         */
         dir = (t > 0) ? 1 : -1;
      } else
         dir = tmcomp (&mytm, &yourtm);
      if (dir != 0) {
         if (t == lo) {
            if (t == time_t_max)
               return WRONG;
            ++t;
            ++lo;
         } else if (t == hi) {
            if (t == time_t_min)
               return WRONG;
            --t;
            --hi;
         }
         if (lo > hi)
            return WRONG;
         if (dir > 0)
            hi = t;
         else
            lo = t;
         continue;
      }
      if (yourtm.tm_isdst < 0 || mytm.tm_isdst == yourtm.tm_isdst)
         break;
      /*
      ** Right time, wrong type.
      ** Hunt for right time, right type.
      ** It's okay to guess wrong since the guess
      ** gets checked.
      */
      sp = (const struct state *) gmtptr;
      if (sp == NULL)
         return WRONG;
      for (i = sp->typecnt - 1; i >= 0; --i) {
         if (sp->ttis[i].tt_isdst != yourtm.tm_isdst)
            continue;
         for (j = sp->typecnt - 1; j >= 0; --j) {
            if (sp->ttis[j].tt_isdst == yourtm.tm_isdst)
               continue;
            newt = t + sp->ttis[j].tt_gmtoff - sp->ttis[i].tt_gmtoff;
            if ((*funcp) (&newt, offset, &mytm) == NULL)
               continue;
            if (tmcomp (&mytm, &yourtm) != 0)
               continue;
            if (mytm.tm_isdst != yourtm.tm_isdst)
               continue;
            /*
            ** We have a match.
            */
            t = newt;
            goto label;
         }
      }
      return WRONG;
   }
label:
   newt = t + saved_seconds;
   if ((newt < t) != (saved_seconds < 0))
      return WRONG;
   t = newt;
   if ((*funcp) (&t, offset, tmp))
      *okayp = true;
   return t;
}

static int64_t
time2 (struct bson_tm *const tmp,
       struct bson_tm *(*const funcp) (const int64_t *, int_fast32_t, struct bson_tm *),
       const int_fast32_t offset,
       int64_t *const okayp)
{
   int64_t t;

   /*
   ** First try without normalization of seconds
   ** (in case tm_sec contains a value associated with a leap second).
   ** If that fails, try with normalization of seconds.
   */
   t = time2sub (tmp, funcp, offset, okayp, false);
   return *okayp ? t : time2sub (tmp, funcp, offset, okayp, true);
}

static int64_t
time1 (struct bson_tm *const tmp,
       struct bson_tm *(*const funcp) (const int64_t *, int_fast32_t, struct bson_tm *),
       const int_fast32_t offset)
{
   register int64_t t;
   register const struct state *sp;
   register int64_t samei, otheri;
   register int64_t sameind, otherind;
   register int64_t i;
   register int64_t nseen;
   int64_t seen[TZ_MAX_TYPES];
   int64_t types[TZ_MAX_TYPES];
   int64_t okay;

   if (tmp == NULL) {
      errno = EINVAL;
      return WRONG;
   }
   if (tmp->tm_isdst > 1)
      tmp->tm_isdst = 1;
   t = time2 (tmp, funcp, offset, &okay);
   if (okay)
      return t;
   if (tmp->tm_isdst < 0)
#ifdef PCTS
      /*
      ** POSIX Conformance Test Suite code courtesy Grant Sullivan.
      */
      tmp->tm_isdst = 0; /* reset to std and try again */
#else
      return t;
#endif /* !defined PCTS */
   /*
   ** We're supposed to assume that somebody took a time of one type
   ** and did some math on it that yielded a "struct tm" that's bad.
   ** We try to divine the type they started from and adjust to the
   ** type they need.
   */
   sp = (const struct state *) gmtptr;
   if (sp == NULL)
      return WRONG;
   for (i = 0; i < sp->typecnt; ++i)
      seen[i] = false;
   nseen = 0;
   for (i = sp->timecnt - 1; i >= 0; --i)
      if (!seen[sp->types[i]]) {
         seen[sp->types[i]] = true;
         types[nseen++] = sp->types[i];
      }
   for (sameind = 0; sameind < nseen; ++sameind) {
      samei = types[sameind];
      if (sp->ttis[samei].tt_isdst != tmp->tm_isdst)
         continue;
      for (otherind = 0; otherind < nseen; ++otherind) {
         otheri = types[otherind];
         if (sp->ttis[otheri].tt_isdst == tmp->tm_isdst)
            continue;
         tmp->tm_sec += sp->ttis[otheri].tt_gmtoff - sp->ttis[samei].tt_gmtoff;
         tmp->tm_isdst = !tmp->tm_isdst;
         t = time2 (tmp, funcp, offset, &okay);
         if (okay)
            return t;
         tmp->tm_sec -= sp->ttis[otheri].tt_gmtoff - sp->ttis[samei].tt_gmtoff;
         tmp->tm_isdst = !tmp->tm_isdst;
      }
   }
   return WRONG;
}

int64_t
_bson_timegm (struct bson_tm *const tmp)
{
   if (tmp != NULL)
      tmp->tm_isdst = 0;
   return time1 (tmp, gmtsub, 0L);
}

