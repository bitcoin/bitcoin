/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ISO8859_1_APRIL_26_2006_1106PM)
#define BOOST_SPIRIT_ISO8859_1_APRIL_26_2006_1106PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <climits>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>

///////////////////////////////////////////////////////////////////////////////
// constants used to classify the single characters
///////////////////////////////////////////////////////////////////////////////
#define BOOST_CC_DIGIT    0x0001
#define BOOST_CC_XDIGIT   0x0002
#define BOOST_CC_ALPHA    0x0004
#define BOOST_CC_CTRL     0x0008
#define BOOST_CC_LOWER    0x0010
#define BOOST_CC_UPPER    0x0020
#define BOOST_CC_SPACE    0x0040
#define BOOST_CC_PUNCT    0x0080

namespace boost { namespace spirit { namespace char_encoding
{
    // The detection of isgraph(), isprint() and isblank() is done programmatically
    // to keep the character type table small. Additionally, these functions are
    // rather seldom used and the programmatic detection is very simple.

    ///////////////////////////////////////////////////////////////////////////
    // ISO 8859-1 character classification table
    //
    // the comments intentionally contain non-ascii characters
    // boostinspect:noascii
    ///////////////////////////////////////////////////////////////////////////
    const unsigned char iso8859_1_char_types[] =
    {
        /* NUL   0   0 */   BOOST_CC_CTRL,
        /* SOH   1   1 */   BOOST_CC_CTRL,
        /* STX   2   2 */   BOOST_CC_CTRL,
        /* ETX   3   3 */   BOOST_CC_CTRL,
        /* EOT   4   4 */   BOOST_CC_CTRL,
        /* ENQ   5   5 */   BOOST_CC_CTRL,
        /* ACK   6   6 */   BOOST_CC_CTRL,
        /* BEL   7   7 */   BOOST_CC_CTRL,
        /* BS    8   8 */   BOOST_CC_CTRL,
        /* HT    9   9 */   BOOST_CC_CTRL|BOOST_CC_SPACE,
        /* NL   10   a */   BOOST_CC_CTRL|BOOST_CC_SPACE,
        /* VT   11   b */   BOOST_CC_CTRL|BOOST_CC_SPACE,
        /* NP   12   c */   BOOST_CC_CTRL|BOOST_CC_SPACE,
        /* CR   13   d */   BOOST_CC_CTRL|BOOST_CC_SPACE,
        /* SO   14   e */   BOOST_CC_CTRL,
        /* SI   15   f */   BOOST_CC_CTRL,
        /* DLE  16  10 */   BOOST_CC_CTRL,
        /* DC1  17  11 */   BOOST_CC_CTRL,
        /* DC2  18  12 */   BOOST_CC_CTRL,
        /* DC3  19  13 */   BOOST_CC_CTRL,
        /* DC4  20  14 */   BOOST_CC_CTRL,
        /* NAK  21  15 */   BOOST_CC_CTRL,
        /* SYN  22  16 */   BOOST_CC_CTRL,
        /* ETB  23  17 */   BOOST_CC_CTRL,
        /* CAN  24  18 */   BOOST_CC_CTRL,
        /* EM   25  19 */   BOOST_CC_CTRL,
        /* SUB  26  1a */   BOOST_CC_CTRL,
        /* ESC  27  1b */   BOOST_CC_CTRL,
        /* FS   28  1c */   BOOST_CC_CTRL,
        /* GS   29  1d */   BOOST_CC_CTRL,
        /* RS   30  1e */   BOOST_CC_CTRL,
        /* US   31  1f */   BOOST_CC_CTRL,
        /* SP   32  20 */   BOOST_CC_SPACE,
        /*  !   33  21 */   BOOST_CC_PUNCT,
        /*  "   34  22 */   BOOST_CC_PUNCT,
        /*  #   35  23 */   BOOST_CC_PUNCT,
        /*  $   36  24 */   BOOST_CC_PUNCT,
        /*  %   37  25 */   BOOST_CC_PUNCT,
        /*  &   38  26 */   BOOST_CC_PUNCT,
        /*  '   39  27 */   BOOST_CC_PUNCT,
        /*  (   40  28 */   BOOST_CC_PUNCT,
        /*  )   41  29 */   BOOST_CC_PUNCT,
        /*  *   42  2a */   BOOST_CC_PUNCT,
        /*  +   43  2b */   BOOST_CC_PUNCT,
        /*  ,   44  2c */   BOOST_CC_PUNCT,
        /*  -   45  2d */   BOOST_CC_PUNCT,
        /*  .   46  2e */   BOOST_CC_PUNCT,
        /*  /   47  2f */   BOOST_CC_PUNCT,
        /*  0   48  30 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  1   49  31 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  2   50  32 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  3   51  33 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  4   52  34 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  5   53  35 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  6   54  36 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  7   55  37 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  8   56  38 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  9   57  39 */   BOOST_CC_DIGIT|BOOST_CC_XDIGIT,
        /*  :   58  3a */   BOOST_CC_PUNCT,
        /*  ;   59  3b */   BOOST_CC_PUNCT,
        /*  <   60  3c */   BOOST_CC_PUNCT,
        /*  =   61  3d */   BOOST_CC_PUNCT,
        /*  >   62  3e */   BOOST_CC_PUNCT,
        /*  ?   63  3f */   BOOST_CC_PUNCT,
        /*  @   64  40 */   BOOST_CC_PUNCT,
        /*  A   65  41 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_UPPER,
        /*  B   66  42 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_UPPER,
        /*  C   67  43 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_UPPER,
        /*  D   68  44 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_UPPER,
        /*  E   69  45 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_UPPER,
        /*  F   70  46 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_UPPER,
        /*  G   71  47 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  H   72  48 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  I   73  49 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  J   74  4a */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  K   75  4b */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  L   76  4c */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  M   77  4d */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  N   78  4e */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  O   79  4f */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  P   80  50 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  Q   81  51 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  R   82  52 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  S   83  53 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  T   84  54 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  U   85  55 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  V   86  56 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  W   87  57 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  X   88  58 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  Y   89  59 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  Z   90  5a */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  [   91  5b */   BOOST_CC_PUNCT,
        /*  \   92  5c */   BOOST_CC_PUNCT,
        /*  ]   93  5d */   BOOST_CC_PUNCT,
        /*  ^   94  5e */   BOOST_CC_PUNCT,
        /*  _   95  5f */   BOOST_CC_PUNCT,
        /*  `   96  60 */   BOOST_CC_PUNCT,
        /*  a   97  61 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_LOWER,
        /*  b   98  62 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_LOWER,
        /*  c   99  63 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_LOWER,
        /*  d  100  64 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_LOWER,
        /*  e  101  65 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_LOWER,
        /*  f  102  66 */   BOOST_CC_ALPHA|BOOST_CC_XDIGIT|BOOST_CC_LOWER,
        /*  g  103  67 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  h  104  68 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  i  105  69 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  j  106  6a */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  k  107  6b */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  l  108  6c */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  m  109  6d */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  n  110  6e */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  o  111  6f */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  p  112  70 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  q  113  71 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  r  114  72 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  s  115  73 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  t  116  74 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  u  117  75 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  v  118  76 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  w  119  77 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  x  120  78 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  y  121  79 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  z  122  7a */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  {  123  7b */   BOOST_CC_PUNCT,
        /*  |  124  7c */   BOOST_CC_PUNCT,
        /*  }  125  7d */   BOOST_CC_PUNCT,
        /*  ~  126  7e */   BOOST_CC_PUNCT,
        /* DEL 127  7f */   BOOST_CC_CTRL,
        /* --  128  80 */   BOOST_CC_CTRL,
        /* --  129  81 */   BOOST_CC_CTRL,
        /* --  130  82 */   BOOST_CC_CTRL,
        /* --  131  83 */   BOOST_CC_CTRL,
        /* --  132  84 */   BOOST_CC_CTRL,
        /* --  133  85 */   BOOST_CC_CTRL,
        /* --  134  86 */   BOOST_CC_CTRL,
        /* --  135  87 */   BOOST_CC_CTRL,
        /* --  136  88 */   BOOST_CC_CTRL,
        /* --  137  89 */   BOOST_CC_CTRL,
        /* --  138  8a */   BOOST_CC_CTRL,
        /* --  139  8b */   BOOST_CC_CTRL,
        /* --  140  8c */   BOOST_CC_CTRL,
        /* --  141  8d */   BOOST_CC_CTRL,
        /* --  142  8e */   BOOST_CC_CTRL,
        /* --  143  8f */   BOOST_CC_CTRL,
        /* --  144  90 */   BOOST_CC_CTRL,
        /* --  145  91 */   BOOST_CC_CTRL,
        /* --  146  92 */   BOOST_CC_CTRL,
        /* --  147  93 */   BOOST_CC_CTRL,
        /* --  148  94 */   BOOST_CC_CTRL,
        /* --  149  95 */   BOOST_CC_CTRL,
        /* --  150  96 */   BOOST_CC_CTRL,
        /* --  151  97 */   BOOST_CC_CTRL,
        /* --  152  98 */   BOOST_CC_CTRL,
        /* --  153  99 */   BOOST_CC_CTRL,
        /* --  154  9a */   BOOST_CC_CTRL,
        /* --  155  9b */   BOOST_CC_CTRL,
        /* --  156  9c */   BOOST_CC_CTRL,
        /* --  157  9d */   BOOST_CC_CTRL,
        /* --  158  9e */   BOOST_CC_CTRL,
        /* --  159  9f */   BOOST_CC_CTRL,
        /*     160  a0 */   BOOST_CC_SPACE,
        /*  �  161  a1 */   BOOST_CC_PUNCT,
        /*  �  162  a2 */   BOOST_CC_PUNCT,
        /*  �  163  a3 */   BOOST_CC_PUNCT,
        /*  �  164  a4 */   BOOST_CC_PUNCT,
        /*  �  165  a5 */   BOOST_CC_PUNCT,
        /*  �  166  a6 */   BOOST_CC_PUNCT,
        /*  �  167  a7 */   BOOST_CC_PUNCT,
        /*  �  168  a8 */   BOOST_CC_PUNCT,
        /*  �  169  a9 */   BOOST_CC_PUNCT,
        /*  �  170  aa */   BOOST_CC_PUNCT,
        /*  �  171  ab */   BOOST_CC_PUNCT,
        /*  �  172  ac */   BOOST_CC_PUNCT,
        /*  �  173  ad */   BOOST_CC_PUNCT,
        /*  �  174  ae */   BOOST_CC_PUNCT,
        /*  �  175  af */   BOOST_CC_PUNCT,
        /*  �  176  b0 */   BOOST_CC_PUNCT,
        /*  �  177  b1 */   BOOST_CC_PUNCT,
        /*  �  178  b2 */   BOOST_CC_DIGIT|BOOST_CC_PUNCT,
        /*  �  179  b3 */   BOOST_CC_DIGIT|BOOST_CC_PUNCT,
        /*  �  180  b4 */   BOOST_CC_PUNCT,
        /*  �  181  b5 */   BOOST_CC_PUNCT,
        /*  �  182  b6 */   BOOST_CC_PUNCT,
        /*  �  183  b7 */   BOOST_CC_PUNCT,
        /*  �  184  b8 */   BOOST_CC_PUNCT,
        /*  �  185  b9 */   BOOST_CC_DIGIT|BOOST_CC_PUNCT,
        /*  �  186  ba */   BOOST_CC_PUNCT,
        /*  �  187  bb */   BOOST_CC_PUNCT,
        /*  �  188  bc */   BOOST_CC_PUNCT,
        /*  �  189  bd */   BOOST_CC_PUNCT,
        /*  �  190  be */   BOOST_CC_PUNCT,
        /*  �  191  bf */   BOOST_CC_PUNCT,
        /*  �  192  c0 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  193  c1 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  194  c2 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  195  c3 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  196  c4 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  197  c5 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  198  c6 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  199  c7 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  200  c8 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  201  c9 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  202  ca */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  203  cb */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  204  cc */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  205  cd */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  206  ce */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  207  cf */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  208  d0 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  209  d1 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  210  d2 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  211  d3 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  212  d4 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  213  d5 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  214  d6 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  215  d7 */   BOOST_CC_PUNCT,
        /*  �  216  d8 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  217  d9 */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  218  da */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  219  db */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  220  dc */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  221  dd */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  222  de */   BOOST_CC_ALPHA|BOOST_CC_UPPER,
        /*  �  223  df */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  224  e0 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  225  e1 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  226  e2 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  227  e3 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  228  e4 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  229  e5 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  230  e6 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  231  e7 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  232  e8 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  233  e9 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  234  ea */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  235  eb */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  236  ec */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  237  ed */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  238  ee */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  239  ef */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  240  f0 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  241  f1 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  242  f2 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  243  f3 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  244  f4 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  245  f5 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  246  f6 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  247  f7 */   BOOST_CC_PUNCT,
        /*  �  248  f8 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  249  f9 */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  250  fa */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  251  fb */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  252  fc */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  253  fd */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  254  fe */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
        /*  �  255  ff */   BOOST_CC_ALPHA|BOOST_CC_LOWER,
    };

    ///////////////////////////////////////////////////////////////////////////
    // ISO 8859-1 character conversion table
    ///////////////////////////////////////////////////////////////////////////
    const unsigned char iso8859_1_char_conversion[] =
    {
        /* NUL   0   0 */   '\0',
        /* SOH   1   1 */   '\0',
        /* STX   2   2 */   '\0',
        /* ETX   3   3 */   '\0',
        /* EOT   4   4 */   '\0',
        /* ENQ   5   5 */   '\0',
        /* ACK   6   6 */   '\0',
        /* BEL   7   7 */   '\0',
        /* BS    8   8 */   '\0',
        /* HT    9   9 */   '\0',
        /* NL   10   a */   '\0',
        /* VT   11   b */   '\0',
        /* NP   12   c */   '\0',
        /* CR   13   d */   '\0',
        /* SO   14   e */   '\0',
        /* SI   15   f */   '\0',
        /* DLE  16  10 */   '\0',
        /* DC1  17  11 */   '\0',
        /* DC2  18  12 */   '\0',
        /* DC3  19  13 */   '\0',
        /* DC4  20  14 */   '\0',
        /* NAK  21  15 */   '\0',
        /* SYN  22  16 */   '\0',
        /* ETB  23  17 */   '\0',
        /* CAN  24  18 */   '\0',
        /* EM   25  19 */   '\0',
        /* SUB  26  1a */   '\0',
        /* ESC  27  1b */   '\0',
        /* FS   28  1c */   '\0',
        /* GS   29  1d */   '\0',
        /* RS   30  1e */   '\0',
        /* US   31  1f */   '\0',
        /* SP   32  20 */   '\0',
        /*  !   33  21 */   '\0',
        /*  "   34  22 */   '\0',
        /*  #   35  23 */   '\0',
        /*  $   36  24 */   '\0',
        /*  %   37  25 */   '\0',
        /*  &   38  26 */   '\0',
        /*  '   39  27 */   '\0',
        /*  (   40  28 */   '\0',
        /*  )   41  29 */   '\0',
        /*  *   42  2a */   '\0',
        /*  +   43  2b */   '\0',
        /*  ,   44  2c */   '\0',
        /*  -   45  2d */   '\0',
        /*  .   46  2e */   '\0',
        /*  /   47  2f */   '\0',
        /*  0   48  30 */   '\0',
        /*  1   49  31 */   '\0',
        /*  2   50  32 */   '\0',
        /*  3   51  33 */   '\0',
        /*  4   52  34 */   '\0',
        /*  5   53  35 */   '\0',
        /*  6   54  36 */   '\0',
        /*  7   55  37 */   '\0',
        /*  8   56  38 */   '\0',
        /*  9   57  39 */   '\0',
        /*  :   58  3a */   '\0',
        /*  ;   59  3b */   '\0',
        /*  <   60  3c */   '\0',
        /*  =   61  3d */   '\0',
        /*  >   62  3e */   '\0',
        /*  ?   63  3f */   '\0',
        /*  @   64  40 */   '\0',
        /*  A   65  41 */   'a',
        /*  B   66  42 */   'b',
        /*  C   67  43 */   'c',
        /*  D   68  44 */   'd',
        /*  E   69  45 */   'e',
        /*  F   70  46 */   'f',
        /*  G   71  47 */   'g',
        /*  H   72  48 */   'h',
        /*  I   73  49 */   'i',
        /*  J   74  4a */   'j',
        /*  K   75  4b */   'k',
        /*  L   76  4c */   'l',
        /*  M   77  4d */   'm',
        /*  N   78  4e */   'n',
        /*  O   79  4f */   'o',
        /*  P   80  50 */   'p',
        /*  Q   81  51 */   'q',
        /*  R   82  52 */   'r',
        /*  S   83  53 */   's',
        /*  T   84  54 */   't',
        /*  U   85  55 */   'u',
        /*  V   86  56 */   'v',
        /*  W   87  57 */   'w',
        /*  X   88  58 */   'x',
        /*  Y   89  59 */   'y',
        /*  Z   90  5a */   'z',
        /*  [   91  5b */   '\0',
        /*  \   92  5c */   '\0',
        /*  ]   93  5d */   '\0',
        /*  ^   94  5e */   '\0',
        /*  _   95  5f */   '\0',
        /*  `   96  60 */   '\0',
        /*  a   97  61 */   'A',
        /*  b   98  62 */   'B',
        /*  c   99  63 */   'C',
        /*  d  100  64 */   'D',
        /*  e  101  65 */   'E',
        /*  f  102  66 */   'F',
        /*  g  103  67 */   'G',
        /*  h  104  68 */   'H',
        /*  i  105  69 */   'I',
        /*  j  106  6a */   'J',
        /*  k  107  6b */   'K',
        /*  l  108  6c */   'L',
        /*  m  109  6d */   'M',
        /*  n  110  6e */   'N',
        /*  o  111  6f */   'O',
        /*  p  112  70 */   'P',
        /*  q  113  71 */   'Q',
        /*  r  114  72 */   'R',
        /*  s  115  73 */   'S',
        /*  t  116  74 */   'T',
        /*  u  117  75 */   'U',
        /*  v  118  76 */   'V',
        /*  w  119  77 */   'W',
        /*  x  120  78 */   'X',
        /*  y  121  79 */   'Y',
        /*  z  122  7a */   'Z',
        /*  {  123  7b */   '\0',
        /*  |  124  7c */   '\0',
        /*  }  125  7d */   '\0',
        /*  ~  126  7e */   '\0',
        /* DEL 127  7f */   '\0',
        /* --  128  80 */   '\0',
        /* --  129  81 */   '\0',
        /* --  130  82 */   '\0',
        /* --  131  83 */   '\0',
        /* --  132  84 */   '\0',
        /* --  133  85 */   '\0',
        /* --  134  86 */   '\0',
        /* --  135  87 */   '\0',
        /* --  136  88 */   '\0',
        /* --  137  89 */   '\0',
        /* --  138  8a */   '\0',
        /* --  139  8b */   '\0',
        /* --  140  8c */   '\0',
        /* --  141  8d */   '\0',
        /* --  142  8e */   '\0',
        /* --  143  8f */   '\0',
        /* --  144  90 */   '\0',
        /* --  145  91 */   '\0',
        /* --  146  92 */   '\0',
        /* --  147  93 */   '\0',
        /* --  148  94 */   '\0',
        /* --  149  95 */   '\0',
        /* --  150  96 */   '\0',
        /* --  151  97 */   '\0',
        /* --  152  98 */   '\0',
        /* --  153  99 */   '\0',
        /* --  154  9a */   '\0',
        /* --  155  9b */   '\0',
        /* --  156  9c */   '\0',
        /* --  157  9d */   '\0',
        /* --  158  9e */   '\0',
        /* --  159  9f */   '\0',
        /*     160  a0 */   '\0',
        /*  �  161  a1 */   '\0',
        /*  �  162  a2 */   '\0',
        /*  �  163  a3 */   '\0',
        /*  �  164  a4 */   '\0',
        /*  �  165  a5 */   '\0',
        /*  �  166  a6 */   '\0',
        /*  �  167  a7 */   '\0',
        /*  �  168  a8 */   '\0',
        /*  �  169  a9 */   '\0',
        /*  �  170  aa */   '\0',
        /*  �  171  ab */   '\0',
        /*  �  172  ac */   '\0',
        /*  �  173  ad */   '\0',
        /*  �  174  ae */   '\0',
        /*  �  175  af */   '\0',
        /*  �  176  b0 */   '\0',
        /*  �  177  b1 */   '\0',
        /*  �  178  b2 */   '\0',
        /*  �  179  b3 */   '\0',
        /*  �  180  b4 */   '\0',
        /*  �  181  b5 */   '\0',
        /*  �  182  b6 */   '\0',
        /*  �  183  b7 */   '\0',
        /*  �  184  b8 */   '\0',
        /*  �  185  b9 */   '\0',
        /*  �  186  ba */   '\0',
        /*  �  187  bb */   '\0',
        /*  �  188  bc */   '\0',
        /*  �  189  bd */   '\0',
        /*  �  190  be */   '\0',
        /*  �  191  bf */   '\0',
        /*  �  192  c0 */   0xe0,
        /*  �  193  c1 */   0xe1,
        /*  �  194  c2 */   0xe2,
        /*  �  195  c3 */   0xe3,
        /*  �  196  c4 */   0xe4,
        /*  �  197  c5 */   0xe5,
        /*  �  198  c6 */   0xe6,
        /*  �  199  c7 */   0xe7,
        /*  �  200  c8 */   0xe8,
        /*  �  201  c9 */   0xe9,
        /*  �  202  ca */   0xea,
        /*  �  203  cb */   0xeb,
        /*  �  204  cc */   0xec,
        /*  �  205  cd */   0xed,
        /*  �  206  ce */   0xee,
        /*  �  207  cf */   0xef,
        /*  �  208  d0 */   0xf0,
        /*  �  209  d1 */   0xf1,
        /*  �  210  d2 */   0xf2,
        /*  �  211  d3 */   0xf3,
        /*  �  212  d4 */   0xf4,
        /*  �  213  d5 */   0xf5,
        /*  �  214  d6 */   0xf6,
        /*  �  215  d7 */   '\0',
        /*  �  216  d8 */   0xf8,
        /*  �  217  d9 */   0xf9,
        /*  �  218  da */   0xfa,
        /*  �  219  db */   0xfb,
        /*  �  220  dc */   0xfc,
        /*  �  221  dd */   0xfd,
        /*  �  222  de */   0xfe,
        /*  �  223  df */   '\0',
        /*  �  224  e0 */   0xc0,
        /*  �  225  e1 */   0xc1,
        /*  �  226  e2 */   0xc2,
        /*  �  227  e3 */   0xc3,
        /*  �  228  e4 */   0xc4,
        /*  �  229  e5 */   0xc5,
        /*  �  230  e6 */   0xc6,
        /*  �  231  e7 */   0xc7,
        /*  �  232  e8 */   0xc8,
        /*  �  233  e9 */   0xc9,
        /*  �  234  ea */   0xca,
        /*  �  235  eb */   0xcb,
        /*  �  236  ec */   0xcc,
        /*  �  237  ed */   0xcd,
        /*  �  238  ee */   0xce,
        /*  �  239  ef */   0xcf,
        /*  �  240  f0 */   0xd0,
        /*  �  241  f1 */   0xd1,
        /*  �  242  f2 */   0xd2,
        /*  �  243  f3 */   0xd3,
        /*  �  244  f4 */   0xd4,
        /*  �  245  f5 */   0xd5,
        /*  �  246  f6 */   0xd6,
        /*  �  247  f7 */   '\0',
        /*  �  248  f8 */   0xd8,
        /*  �  249  f9 */   0xd9,
        /*  �  250  fa */   0xda,
        /*  �  251  fb */   0xdb,
        /*  �  252  fc */   0xdc,
        /*  �  253  fd */   0xdd,
        /*  �  254  fe */   0xde,
        /*  �  255  ff */   '\0',
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Test characters for specified conditions (using iso8859-1)
    ///////////////////////////////////////////////////////////////////////////
    struct iso8859_1
    {
        typedef unsigned char char_type;
        typedef unsigned char classify_type;

        static bool
        isascii_(int ch)
        {
            return 0 == (ch & ~0x7f);
        }

        static bool
        ischar(int ch)
        {
            // iso8859.1 uses all 8 bits
            // we have to watch out for sign extensions
            return (0 == (ch & ~0xff) || ~0 == (ch | 0xff)) != 0;
        }

        // *** Note on assertions: The precondition is that the calls to
        // these functions do not violate the required range of ch (type int)
        // which is that strict_ischar(ch) should be true. It is the
        // responsibility of the caller to make sure this precondition is not
        // violated.

        static bool
        strict_ischar(int ch)
        {
            return ch >= 0 && ch <= 255;
        }

        static bool
        isalnum(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_ALPHA)
                || (iso8859_1_char_types[ch] & BOOST_CC_DIGIT);
        }

        static bool
        isalpha(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_ALPHA) != 0;
        }

        static bool
        isdigit(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_DIGIT) != 0;
        }

        static bool
        isxdigit(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_XDIGIT) != 0;
        }

        static bool
        iscntrl(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_CTRL) != 0;
        }

        static bool
        isgraph(int ch)
        {
            return ('\x21' <= ch && ch <= '\x7e') || ('\xa1' <= ch && ch <= '\xff');
        }

        static bool
        islower(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_LOWER) != 0;
        }

        static bool
        isprint(int ch)
        {
            return ('\x20' <= ch && ch <= '\x7e') || ('\xa0' <= ch && ch <= '\xff');
        }

        static bool
        ispunct(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_PUNCT) != 0;
        }

        static bool
        isspace(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_SPACE) != 0;
        }

        static bool
        isblank BOOST_PREVENT_MACRO_SUBSTITUTION (int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return ('\x09' == ch || '\x20' == ch || '\xa0' == ch);
        }

        static bool
        isupper(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return (iso8859_1_char_types[ch] & BOOST_CC_UPPER) != 0;
        }

    ///////////////////////////////////////////////////////////////////////////
    //  Simple character conversions
    ///////////////////////////////////////////////////////////////////////////

        static int
        tolower(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return isupper(ch) && '\0' != iso8859_1_char_conversion[ch] ?
                iso8859_1_char_conversion[ch] : ch;
        }

        static int
        toupper(int ch)
        {
            BOOST_ASSERT(strict_ischar(ch));
            return islower(ch) && '\0' != iso8859_1_char_conversion[ch] ?
                iso8859_1_char_conversion[ch] : ch;
        }

        static ::boost::uint32_t
        toucs4(int ch)
        {
            // The first 256 characters in Unicode and the UCS are
            // identical to those in ISO/IEC-8859-1.
            BOOST_ASSERT(strict_ischar(ch));
            return ch;
        }
    };

}}}

///////////////////////////////////////////////////////////////////////////////
// undefine macros
///////////////////////////////////////////////////////////////////////////////
#undef BOOST_CC_DIGIT
#undef BOOST_CC_XDIGIT
#undef BOOST_CC_ALPHA
#undef BOOST_CC_CTRL
#undef BOOST_CC_LOWER
#undef BOOST_CC_UPPER
#undef BOOST_CC_PUNCT
#undef BOOST_CC_SPACE

#endif

