#include <sys/param.h>
#include <sys/socket.h>

// Since the cmsg(3) macros are macros instead of functions, they aren't
// available to FFI.  libc must reimplement them, which is error-prone.  This
// file provides FFI access to the actual macros so they can be tested against
// the Rust reimplementations.

struct cmsghdr *cmsg_firsthdr(struct msghdr *msgh) {
	return CMSG_FIRSTHDR(msgh);
}

struct cmsghdr *cmsg_nxthdr(struct msghdr *msgh, struct cmsghdr *cmsg) {
	return CMSG_NXTHDR(msgh, cmsg);
}

size_t cmsg_space(size_t length) {
	return CMSG_SPACE(length);
}

size_t cmsg_len(size_t length) {
	return CMSG_LEN(length);
}

unsigned char *cmsg_data(struct cmsghdr *cmsg) {
	return CMSG_DATA(cmsg);
}

