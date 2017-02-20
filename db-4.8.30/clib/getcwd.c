/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifdef HAVE_SYSTEM_INCLUDE_FILES
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#endif

#define	ISDOT(dp) \
	(dp->d_name[0] == '.' && (dp->d_name[1] == '\0' || \
	    (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))

#ifndef dirfd
#define	dirfd(dirp)     ((dirp)->dd_fd)
#endif

/*
 * getcwd --
 *	Get the current working directory.
 *
 * PUBLIC: #ifndef HAVE_GETCWD
 * PUBLIC: char *getcwd __P((char *, size_t));
 * PUBLIC: #endif
 */
char *
getcwd(pt, size)
	char *pt;
	size_t size;
{
	register struct dirent *dp;
	register DIR *dir;
	register dev_t dev;
	register ino_t ino;
	register int first;
	register char *bpt, *bup;
	struct stat s;
	dev_t root_dev;
	ino_t root_ino;
	size_t ptsize, upsize;
	int ret, save_errno;
	char *ept, *eup, *up;

	/*
	 * If no buffer specified by the user, allocate one as necessary.
	 * If a buffer is specified, the size has to be non-zero.  The path
	 * is built from the end of the buffer backwards.
	 */
	if (pt) {
		ptsize = 0;
		if (!size) {
			__os_set_errno(EINVAL);
			return (NULL);
		}
		if (size == 1) {
			__os_set_errno(ERANGE);
			return (NULL);
		}
		ept = pt + size;
	} else {
		if ((ret =
		    __os_malloc(NULL, ptsize = 1024 - 4, &pt)) != 0) {
			__os_set_errno(ret);
			return (NULL);
		}
		ept = pt + ptsize;
	}
	bpt = ept - 1;
	*bpt = '\0';

	/*
	 * Allocate bytes (1024 - malloc space) for the string of "../"'s.
	 * Should always be enough (it's 340 levels).  If it's not, allocate
	 * as necessary.  Special case the first stat, it's ".", not "..".
	 */
	if ((ret = __os_malloc(NULL, upsize = 1024 - 4, &up)) != 0)
		goto err;
	eup = up + 1024;
	bup = up;
	up[0] = '.';
	up[1] = '\0';

	/* Save root values, so know when to stop. */
	if (stat("/", &s))
		goto err;
	root_dev = s.st_dev;
	root_ino = s.st_ino;

	__os_set_errno(0);		/* XXX readdir has no error return. */

	for (first = 1;; first = 0) {
		/* Stat the current level. */
		if (lstat(up, &s))
			goto err;

		/* Save current node values. */
		ino = s.st_ino;
		dev = s.st_dev;

		/* Check for reaching root. */
		if (root_dev == dev && root_ino == ino) {
			*--bpt = PATH_SEPARATOR[0];
			/*
			 * It's unclear that it's a requirement to copy the
			 * path to the beginning of the buffer, but it's always
			 * been that way and stuff would probably break.
			 */
			bcopy(bpt, pt, ept - bpt);
			__os_free(NULL, up);
			return (pt);
		}

		/*
		 * Build pointer to the parent directory, allocating memory
		 * as necessary.  Max length is 3 for "../", the largest
		 * possible component name, plus a trailing NULL.
		 */
		if (bup + 3  + MAXNAMLEN + 1 >= eup) {
			if (__os_realloc(NULL, upsize *= 2, &up) != 0)
				goto err;
			bup = up;
			eup = up + upsize;
		}
		*bup++ = '.';
		*bup++ = '.';
		*bup = '\0';

		/* Open and stat parent directory. */
		if (!(dir = opendir(up)) || fstat(dirfd(dir), &s))
			goto err;

		/* Add trailing slash for next directory. */
		*bup++ = PATH_SEPARATOR[0];

		/*
		 * If it's a mount point, have to stat each element because
		 * the inode number in the directory is for the entry in the
		 * parent directory, not the inode number of the mounted file.
		 */
		save_errno = 0;
		if (s.st_dev == dev) {
			for (;;) {
				if (!(dp = readdir(dir)))
					goto notfound;
				if (dp->d_fileno == ino)
					break;
			}
		} else
			for (;;) {
				if (!(dp = readdir(dir)))
					goto notfound;
				if (ISDOT(dp))
					continue;
				bcopy(dp->d_name, bup, dp->d_namlen + 1);

				/* Save the first error for later. */
				if (lstat(up, &s)) {
					if (save_errno == 0)
						save_errno = __os_get_errno();
					__os_set_errno(0);
					continue;
				}
				if (s.st_dev == dev && s.st_ino == ino)
					break;
			}

		/*
		 * Check for length of the current name, preceding slash,
		 * leading slash.
		 */
		if (bpt - pt < dp->d_namlen + (first ? 1 : 2)) {
			size_t len, off;

			if (!ptsize) {
				__os_set_errno(ERANGE);
				goto err;
			}
			off = bpt - pt;
			len = ept - bpt;
			if (__os_realloc(NULL, ptsize *= 2, &pt) != 0)
				goto err;
			bpt = pt + off;
			ept = pt + ptsize;
			bcopy(bpt, ept - len, len);
			bpt = ept - len;
		}
		if (!first)
			*--bpt = PATH_SEPARATOR[0];
		bpt -= dp->d_namlen;
		bcopy(dp->d_name, bpt, dp->d_namlen);
		(void)closedir(dir);

		/* Truncate any file name. */
		*bup = '\0';
	}

notfound:
	/*
	 * If readdir set errno, use it, not any saved error; otherwise,
	 * didn't find the current directory in its parent directory, set
	 * errno to ENOENT.
	 */
	if (__os_get_errno_ret_zero() == 0)
		__os_set_errno(save_errno == 0 ? ENOENT : save_errno);
	/* FALLTHROUGH */
err:
	if (ptsize)
		__os_free(NULL, pt);
	__os_free(NULL, up);
	return (NULL);
}
