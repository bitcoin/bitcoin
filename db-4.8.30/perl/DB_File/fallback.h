#define PERL_constant_NOTFOUND  1
#define PERL_constant_NOTDEF    2
#define PERL_constant_ISIV  3
#define PERL_constant_ISNO  4
#define PERL_constant_ISNV  5
#define PERL_constant_ISPV  6
#define PERL_constant_ISPVN 7
#define PERL_constant_ISSV  8
#define PERL_constant_ISUNDEF   9
#define PERL_constant_ISUV  10
#define PERL_constant_ISYES 11

#ifndef NVTYPE
typedef double NV; /* 5.6 and later define NVTYPE, and typedef NV to it.  */
#endif
#ifndef aTHX_
#define aTHX_ /* 5.6 or later define this for threading support.  */
#endif
#ifndef pTHX_
#define pTHX_ /* 5.6 or later define this for threading support.  */
#endif

static int
constant_6 (pTHX_ const char *name, IV *iv_return) {
  /* When generated this function returned values for the list of names given
     here.  However, subsequent manual editing may have added or removed some.
     DB_TXN R_LAST R_NEXT R_PREV */
  /* Offset 2 gives the best switch position.  */
  switch (name[2]) {
  case 'L':
    if (memEQ(name, "R_LAST", 6)) {
    /*                 ^         */
#ifdef R_LAST
      *iv_return = R_LAST;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'N':
    if (memEQ(name, "R_NEXT", 6)) {
    /*                 ^         */
#ifdef R_NEXT
      *iv_return = R_NEXT;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'P':
    if (memEQ(name, "R_PREV", 6)) {
    /*                 ^         */
#ifdef R_PREV
      *iv_return = R_PREV;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case '_':
    if (memEQ(name, "DB_TXN", 6)) {
    /*                 ^         */
#ifdef DB_TXN
      *iv_return = DB_TXN;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

static int
constant_7 (pTHX_ const char *name, IV *iv_return) {
  /* When generated this function returned values for the list of names given
     here.  However, subsequent manual editing may have added or removed some.
     DB_LOCK R_FIRST R_NOKEY */
  /* Offset 3 gives the best switch position.  */
  switch (name[3]) {
  case 'I':
    if (memEQ(name, "R_FIRST", 7)) {
    /*                  ^         */
#ifdef R_FIRST
      *iv_return = R_FIRST;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'L':
    if (memEQ(name, "DB_LOCK", 7)) {
    /*                  ^         */
#ifdef DB_LOCK
      *iv_return = DB_LOCK;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'O':
    if (memEQ(name, "R_NOKEY", 7)) {
    /*                  ^         */
#ifdef R_NOKEY
      *iv_return = R_NOKEY;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

static int
constant_8 (pTHX_ const char *name, IV *iv_return) {
  /* When generated this function returned values for the list of names given
     here.  However, subsequent manual editing may have added or removed some.
     DB_SHMEM R_CURSOR R_IAFTER */
  /* Offset 5 gives the best switch position.  */
  switch (name[5]) {
  case 'M':
    if (memEQ(name, "DB_SHMEM", 8)) {
    /*                    ^        */
#ifdef DB_SHMEM
      *iv_return = DB_SHMEM;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'S':
    if (memEQ(name, "R_CURSOR", 8)) {
    /*                    ^        */
#ifdef R_CURSOR
      *iv_return = R_CURSOR;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'T':
    if (memEQ(name, "R_IAFTER", 8)) {
    /*                    ^        */
#ifdef R_IAFTER
      *iv_return = R_IAFTER;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

static int
constant_9 (pTHX_ const char *name, IV *iv_return) {
  /* When generated this function returned values for the list of names given
     here.  However, subsequent manual editing may have added or removed some.
     HASHMAGIC RET_ERROR R_IBEFORE */
  /* Offset 7 gives the best switch position.  */
  switch (name[7]) {
  case 'I':
    if (memEQ(name, "HASHMAGIC", 9)) {
    /*                      ^       */
#ifdef HASHMAGIC
      *iv_return = HASHMAGIC;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'O':
    if (memEQ(name, "RET_ERROR", 9)) {
    /*                      ^       */
#ifdef RET_ERROR
      *iv_return = RET_ERROR;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'R':
    if (memEQ(name, "R_IBEFORE", 9)) {
    /*                      ^       */
#ifdef R_IBEFORE
      *iv_return = R_IBEFORE;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

static int
constant_10 (pTHX_ const char *name, IV *iv_return) {
  /* When generated this function returned values for the list of names given
     here.  However, subsequent manual editing may have added or removed some.
     BTREEMAGIC R_FIXEDLEN R_SNAPSHOT __R_UNUSED */
  /* Offset 5 gives the best switch position.  */
  switch (name[5]) {
  case 'E':
    if (memEQ(name, "R_FIXEDLEN", 10)) {
    /*                    ^           */
#ifdef R_FIXEDLEN
      *iv_return = R_FIXEDLEN;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'M':
    if (memEQ(name, "BTREEMAGIC", 10)) {
    /*                    ^           */
#ifdef BTREEMAGIC
      *iv_return = BTREEMAGIC;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'N':
    if (memEQ(name, "__R_UNUSED", 10)) {
    /*                    ^           */
#ifdef __R_UNUSED
      *iv_return = __R_UNUSED;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'P':
    if (memEQ(name, "R_SNAPSHOT", 10)) {
    /*                    ^           */
#ifdef R_SNAPSHOT
      *iv_return = R_SNAPSHOT;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

static int
constant_11 (pTHX_ const char *name, IV *iv_return) {
  /* When generated this function returned values for the list of names given
     here.  However, subsequent manual editing may have added or removed some.
     HASHVERSION RET_SPECIAL RET_SUCCESS R_RECNOSYNC R_SETCURSOR */
  /* Offset 10 gives the best switch position.  */
  switch (name[10]) {
  case 'C':
    if (memEQ(name, "R_RECNOSYNC", 11)) {
    /*                         ^       */
#ifdef R_RECNOSYNC
      *iv_return = R_RECNOSYNC;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'L':
    if (memEQ(name, "RET_SPECIAL", 11)) {
    /*                         ^       */
#ifdef RET_SPECIAL
      *iv_return = RET_SPECIAL;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'N':
    if (memEQ(name, "HASHVERSION", 11)) {
    /*                         ^       */
#ifdef HASHVERSION
      *iv_return = HASHVERSION;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'R':
    if (memEQ(name, "R_SETCURSOR", 11)) {
    /*                         ^       */
#ifdef R_SETCURSOR
      *iv_return = R_SETCURSOR;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 'S':
    if (memEQ(name, "RET_SUCCESS", 11)) {
    /*                         ^       */
#ifdef RET_SUCCESS
      *iv_return = RET_SUCCESS;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

static int
constant (pTHX_ const char *name, STRLEN len, IV *iv_return) {
  /* Initially switch on the length of the name.  */
  /* When generated this function returned values for the list of names given
     in this section of perl code.  Rather than manually editing these functions
     to add or remove constants, which would result in this comment and section
     of code becoming inaccurate, we recommend that you edit this section of
     code, and use it to regenerate a new set of constant functions which you
     then use to replace the originals.

     Regenerate these constant functions by feeding this entire source file to
     perl -x

#!bleedperl -w
use ExtUtils::Constant qw (constant_types C_constant XS_constant);

my $types = {map {($_, 1)} qw(IV)};
my @names = (qw(BTREEMAGIC BTREEVERSION DB_LOCK DB_SHMEM DB_TXN HASHMAGIC
           HASHVERSION MAX_PAGE_NUMBER MAX_PAGE_OFFSET MAX_REC_NUMBER
           RET_ERROR RET_SPECIAL RET_SUCCESS R_CURSOR R_DUP R_FIRST
           R_FIXEDLEN R_IAFTER R_IBEFORE R_LAST R_NEXT R_NOKEY
           R_NOOVERWRITE R_PREV R_RECNOSYNC R_SETCURSOR R_SNAPSHOT
           __R_UNUSED));

print constant_types(); # macro defs
foreach (C_constant ("DB_File", 'constant', 'IV', $types, undef, 3, @names) ) {
    print $_, "\n"; # C constant subs
}
print "#### XS Section:\n";
print XS_constant ("DB_File", $types);
__END__
   */

  switch (len) {
  case 5:
    if (memEQ(name, "R_DUP", 5)) {
#ifdef R_DUP
      *iv_return = R_DUP;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 6:
    return constant_6 (aTHX_ name, iv_return);
    break;
  case 7:
    return constant_7 (aTHX_ name, iv_return);
    break;
  case 8:
    return constant_8 (aTHX_ name, iv_return);
    break;
  case 9:
    return constant_9 (aTHX_ name, iv_return);
    break;
  case 10:
    return constant_10 (aTHX_ name, iv_return);
    break;
  case 11:
    return constant_11 (aTHX_ name, iv_return);
    break;
  case 12:
    if (memEQ(name, "BTREEVERSION", 12)) {
#ifdef BTREEVERSION
      *iv_return = BTREEVERSION;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 13:
    if (memEQ(name, "R_NOOVERWRITE", 13)) {
#ifdef R_NOOVERWRITE
      *iv_return = R_NOOVERWRITE;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 14:
    if (memEQ(name, "MAX_REC_NUMBER", 14)) {
#ifdef MAX_REC_NUMBER
      *iv_return = MAX_REC_NUMBER;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 15:
    /* Names all of length 15.  */
    /* MAX_PAGE_NUMBER MAX_PAGE_OFFSET */
    /* Offset 9 gives the best switch position.  */
    switch (name[9]) {
    case 'N':
      if (memEQ(name, "MAX_PAGE_NUMBER", 15)) {
      /*                        ^            */
#ifdef MAX_PAGE_NUMBER
        *iv_return = MAX_PAGE_NUMBER;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    case 'O':
      if (memEQ(name, "MAX_PAGE_OFFSET", 15)) {
      /*                        ^            */
#ifdef MAX_PAGE_OFFSET
        *iv_return = MAX_PAGE_OFFSET;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

