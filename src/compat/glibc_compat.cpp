// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/syscoin-config.h>
#endif

#include <cstddef>
#include <cstdint>
// SYSCOIN
#define _GNU_SOURCE 1
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/cdefs.h>
#ifndef WIN32
#include <fcntl.h>
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_GETRANDOM
#include <sys/syscall.h>
#include <linux/random.h>
#endif
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#if defined(__i386__) || defined(__arm__)

extern "C" int64_t __udivmoddi4(uint64_t u, uint64_t v, uint64_t* rp);

extern "C" int64_t __wrap___divmoddi4(int64_t u, int64_t v, int64_t* rp)
{
    int32_t c1 = 0, c2 = 0;
    int64_t uu = u, vv = v;
    int64_t w;
    int64_t r;

    if (uu < 0) {
        c1 = ~c1, c2 = ~c2, uu = -uu;
    }
    if (vv < 0) {
        c1 = ~c1, vv = -vv;
    }

    w = __udivmoddi4(uu, vv, (uint64_t*)&r);
    if (c1)
        w = -w;
    if (c2)
        r = -r;

    *rp = r;
    return w;
}
#endif

extern "C" float log2f_old(float x);
#ifdef __i386__
__asm(".symver log2f_old,log2f@GLIBC_2.1");
#elif defined(__amd64__)
__asm(".symver log2f_old,log2f@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver log2f_old,log2f@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver log2f_old,log2f@GLIBC_2.17");
#elif defined(__powerpc64__)
#  ifdef WORDS_BIGENDIAN
__asm(".symver log2f_old,log2f@GLIBC_2.3");
#  else
__asm(".symver log2f_old,log2f@GLIBC_2.17");
#  endif
#elif defined(__riscv)
__asm(".symver log2f_old,log2f@GLIBC_2.27");
#endif
extern "C" float __wrap_log2f(float x)
{
    return log2f_old(x);
}
// SYSCOIN wrapper for getrandom 2.25 glibc compatibility for up to 2.17 backwards
#ifndef WIN32
/** Fallback: get 32 bytes of system entropy from /dev/urandom. The most
 * compatible way to get cryptographic randomness on UNIX-ish platforms.
 */
static ssize_t GetDevURandom(unsigned char *ent32, size_t length)
{
    int f = open("/dev/urandom", O_RDONLY);
    if (f == -1) {
        return -1;
    }
    size_t have = 0;
    do {
        ssize_t n = read(f, ent32 + have, length - have);
        if (n <= 0 || n + have > length) {
            close(f);
            return n;
        }
        have += n;
    } while (have < length);
    close(f);
    return (ssize_t)have;
}
#endif

/* Write LENGTH bytes of randomness starting at BUFFER.  Return 0 on
    success and -1 on failure.  */
extern "C" ssize_t __wrap_getrandom (void *buffer, size_t length, unsigned int flags)
    {
        ssize_t rv = -1;
        #if defined(HAVE_SYS_GETRANDOM)
        /* Linux. From the getrandom(2) man page:
        * "If the urandom source has been initialized, reads of up to 256 bytes
        * will always return as many bytes as requested and will not be
        * interrupted by signals."
        */
        rv = syscall(SYS_getrandom, buffer, length, flags);
        if ((size_t)rv != length) {
            if (rv < 0 && errno == ENOSYS) {
                /* Fallback for kernel <3.17: the return value will be -1 and errno
                * ENOSYS if the syscall is not available, in that case fall back
                * to /dev/urandom.
                */
                return GetDevURandom((unsigned char*)buffer, length);
            }
        }
        #else 
            return GetDevURandom((unsigned char*)buffer, length);
        #endif
        return rv;
    }

    // fmemopen


typedef struct fmemopen_cookie_struct fmemopen_cookie_t;
struct fmemopen_cookie_struct
{
  char        *buffer;   /* memory buffer.  */
  int         mybuffer;  /* allocated my buffer?  */
  int         append;    /* buffer open for append?  */
  size_t      size;      /* buffer length in bytes.  */
  off_t     pos;       /* current position at the buffer.  */
  size_t      maxpos;    /* max position in buffer.  */
};


static ssize_t
fmemopen_read (void *cookie, char *b, size_t s)
{
  fmemopen_cookie_t *c = (fmemopen_cookie_t *) cookie;

  if (c->pos + s > c->maxpos)
    {
      s = c->maxpos - c->pos;
      if ((size_t) c->pos > c->maxpos)
	s = 0;
    }

  memcpy (b, &(c->buffer[c->pos]), s);

  c->pos += s;

  return s;
}


static ssize_t
fmemopen_write (void *cookie, const char *b, size_t s)
{
  fmemopen_cookie_t *c = (fmemopen_cookie_t *) cookie;;
  off_t pos = c->append ? c->maxpos : c->pos;
  int addnullc = (s == 0 || b[s - 1] != '\0');

  if (pos + s > c->size)
    {
      if ((size_t) (c->pos + addnullc) >= c->size)
	{
	  errno = ENOSPC;
	  return 0;
	}
      s = c->size - pos;
    }

  memcpy (&(c->buffer[pos]), b, s);

  c->pos = pos + s;
  if ((size_t) c->pos > c->maxpos)
    {
      c->maxpos = c->pos;
      if (c->maxpos < c->size && addnullc)
	c->buffer[c->maxpos] = '\0';
      /* A null byte is written in a stream open for update iff it fits.  */
      else if (c->append == 0 && addnullc != 0)
	c->buffer[c->size-1] = '\0';
    }

  return s;
}


static int
fmemopen_seek (void *cookie, off_t *p, int w)
{
  off_t np;
  fmemopen_cookie_t *c = (fmemopen_cookie_t *) cookie;

  switch (w)
    {
    case SEEK_SET:
      np = *p;
      break;

    case SEEK_CUR:
      np = c->pos + *p;
      break;

    case SEEK_END:
      np = c->maxpos + *p;
      break;

    default:
      return -1;
    }

  if (np < 0 || (size_t) np > c->size)
    {
      errno = EINVAL;
      return -1;
    }

  *p = c->pos = np;

  return 0;
}


static int
fmemopen_close (void *cookie)
{
  fmemopen_cookie_t *c = (fmemopen_cookie_t *) cookie;

  if (c->mybuffer)
    free (c->buffer);
  free (c);

  return 0;
}

extern "C" FILE * __wrap_fmemopen (void *buf, size_t len, const char *mode)
{
  cookie_io_functions_t iof;
  fmemopen_cookie_t *c;
  FILE *result;

  c = (fmemopen_cookie_t *) calloc (sizeof (fmemopen_cookie_t), 1);
  if (c == NULL)
    return NULL;

  c->mybuffer = (buf == NULL);

  if (c->mybuffer)
    {
      c->buffer = (char *) malloc (len);
      if (c->buffer == NULL)
	{
	  free (c);
	  return NULL;
	}
      c->buffer[0] = '\0';
    }
  else
    {
      if (likely ((uintptr_t) len > -(uintptr_t) buf))
	{
	  free (c);
	  errno = EINVAL;
	  return NULL;
	}

      c->buffer = (char*)buf;

      /* POSIX states that w+ mode should truncate the buffer.  */
      if (mode[0] == 'w' && mode[1] == '+')
	c->buffer[0] = '\0';

      if (mode[0] == 'a')
        c->maxpos = strnlen (c->buffer, len);
    }


  /* Mode   |  starting position (cookie::pos) |          size (cookie::size)
     ------ |----------------------------------|-----------------------------
     read   |          beginning of the buffer |                size argument
     write  |          beginning of the buffer |                         zero
     append |    first null or size buffer + 1 |  first null or size argument
   */

  c->size = len;

  if (mode[0] == 'r')
    c->maxpos = len;

  c->append = mode[0] == 'a';
  if (c->append)
    c->pos = c->maxpos;
  else
    c->pos = 0;

  iof.read = fmemopen_read;
  iof.write = fmemopen_write;
  iof.seek = fmemopen_seek;
  iof.close = fmemopen_close;

  result = fopencookie (c, mode, iof);
  if (likely (result == NULL))
    {
      if (c->mybuffer)
	free (c->buffer);

      free (c);
    }

  return result;
}