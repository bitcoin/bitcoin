/* This is currently only known to be implemented on linux. *
 * enable by defining USE_POSIX_CAPABILITIES at build time. *
 * You will then need to either make the bitcoin/bitcoind   *
 * binary setuid root (bad) or setcap setpcap=eip; (good)   *
 *                                                          *
 * Solaris may have an implementation but this may be linux *
 * 2.6.32+ specific. Sorry. You will need libcap2.          */

#ifdef USE_POSIX_CAPABILITIES

#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/types.h>

// This is current as of linux 2.6.38.5
#ifndef CAP_LAST_CAP
#define CAP_LAST_CAP CAP_AUDIT_CONTROL
#endif

// These defines should be removable once there is a working autotools that can
// find kernel headers. linux/securebits.h
//#include <linux/securebits.h>
/* !_LINUX_SECUREBITS_H */
#ifndef _LINUX_SECUREBITS_H
#define _LINUX_SECUREBITS_H 1
#define SECUREBITS_DEFAULT 0x00000000
#define SECURE_NOROOT			0
#define SECURE_NOROOT_LOCKED		1  /* make bit-0 immutable */
#define SECURE_NO_SETUID_FIXUP		2
#define SECURE_NO_SETUID_FIXUP_LOCKED	3  /* make bit-2 immutable */
#define SECURE_KEEP_CAPS		4
#define SECURE_KEEP_CAPS_LOCKED		5  /* make bit-4 immutable */
#define issecure_mask(X)	(1 << (X))
#define issecure(X)		(issecure_mask(X) & current_cred_xxx(securebits))
#define SECURE_ALL_BITS		(issecure_mask(SECURE_NOROOT) | \
				 issecure_mask(SECURE_NO_SETUID_FIXUP) | \
				 issecure_mask(SECURE_KEEP_CAPS))
#define SECURE_ALL_LOCKS	(SECURE_ALL_BITS << 1)
#endif /* !_LINUX_SECUREBITS_H */

#ifdef __cplusplus
extern "C" {
#endif

extern int cCap_lock(void);
int cCap_clear_bnd(__u32 bnd);

int cCap_lock(void) {
  int error = 0;
  cap_flag_value_t check;
  cap_t state, newstate;

  if ((state = cap_get_proc()) == NULL)
    return -1;

  if ((newstate = cap_dup(state)) != NULL)
    if (cap_clear(newstate) != 0)
      return -1;

  /* If we have CAP_SETPCAP clear all capabilities and the bounding set. */
  if(
      (error += cap_get_flag(state,CAP_SETPCAP,CAP_EFFECTIVE,&check) == 0 )
      && check == CAP_SET
    ) {

    error += prctl(PR_SET_SECUREBITS,
        SECURE_NO_SETUID_FIXUP|SECURE_NO_SETUID_FIXUP_LOCKED
        |SECURE_NOROOT|SECURE_NOROOT_LOCKED
        );
    error += cCap_clear_bnd(0);
    error += cap_set_proc(newstate);
  }

  cap_free(newstate);
  cap_free(state);

  return error;
}

int cCap_clear_bnd(__u32 bnd) {
  unsigned j;
  int error = 0;

  /* Remove all capabilities that are not in
   * bnd from bounding set! */
  for(j=0;j <= CAP_LAST_CAP; j++) {
    if (prctl(PR_CAPBSET_READ, j) >=0) {
      if(!(bnd & CAP_TO_MASK(j)))
        error += prctl(PR_CAPBSET_DROP,j);
    }
  }
  return error;
}

#ifdef __cplusplus
}
#endif

#endif

