(use-modules (gnu)
             (gnu packages)
             (gnu packages autotools)
             (gnu packages base)
             (gnu packages bash)
             (gnu packages check)
             (gnu packages commencement)
             (gnu packages compression)
             (gnu packages cross-base)
             (gnu packages file)
             (gnu packages gawk)
             (gnu packages gcc)
             (gnu packages linux)
             (gnu packages perl)
             (gnu packages pkg-config)
             (gnu packages python)
             (gnu packages shells)
             (guix build-system trivial)
             (guix gexp)
             (guix packages)
             (guix profiles)
             (guix utils))

(define (make-gcc-rpath-link xgcc)
  "Given a XGCC package, return a modified package that replace each instance of
-rpath in the default system spec that's inserted by Guix with -rpath-link"
  (package
   (inherit xgcc)
   (arguments
    (substitute-keyword-arguments (package-arguments xgcc)
      ((#:phases phases)
       `(modify-phases ,phases
          (add-after 'pre-configure 'replace-rpath-with-rpath-link
            (lambda _
              (substitute* (cons "gcc/config/rs6000/sysv4.h"
                                 (find-files "gcc/config"
                                             "^gnu-user.*\\.h$"))
                (("-rpath=") "-rpath-link="))
              #t))))))))

(define (make-cross-toolchain target
                              base-gcc-for-libc
                              base-kernel-headers
                              base-libc
                              base-gcc)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (cross-binutils target))
         ;; 1. Build a cross-compiling gcc without targeting any libc, derived
         ;; from BASE-GCC-FOR-LIBC
         (xgcc-sans-libc (cross-gcc target
                                    #:xgcc base-gcc-for-libc
                                    #:xbinutils xbinutils))
         ;; 2. Build cross-compiled kernel headers with XGCC-SANS-LIBC, derived
         ;; from BASE-KERNEL-HEADERS
         (xkernel (cross-kernel-headers target
                                        base-kernel-headers
                                        xgcc-sans-libc
                                        xbinutils))
         ;; 3. Build a cross-compiled libc with XGCC-SANS-LIBC and XKERNEL,
         ;; derived from BASE-LIBC
         (xlibc (cross-libc target
                            base-libc
                            xgcc-sans-libc
                            xbinutils
                            xkernel))
         ;; 4. Build a cross-compiling gcc targeting XLIBC, derived from
         ;; BASE-GCC
         (xgcc (cross-gcc target
                          #:xgcc base-gcc
                          #:xbinutils xbinutils
                          #:libc xlibc)))
    ;; Define a meta-package that propagates the resulting XBINUTILS, XLIBC, and
    ;; XGCC
    (package
      (name (string-append target "-toolchain"))
      (version (package-version xgcc))
      (source #f)
      (build-system trivial-build-system)
      (arguments '(#:builder (begin (mkdir %output) #t)))
      (propagated-inputs
       `(("binutils" ,xbinutils)
         ("libc" ,xlibc)
         ("libc:static" ,xlibc "static")
         ("gcc" ,xgcc)))
      (synopsis (string-append "Complete GCC tool chain for " target))
      (description (string-append "This package provides a complete GCC tool
chain for " target " development."))
      (home-page (package-home-page xgcc))
      (license (package-license xgcc)))))

(define* (make-bitcoin-cross-toolchain target
                                  #:key
                                  (base-gcc-for-libc gcc-5)
                                  (base-kernel-headers linux-libre-headers-4.19)
                                  (base-libc glibc-2.27)
                                  (base-gcc (make-gcc-rpath-link gcc-9)))
  "Convenience wrapper around MAKE-CROSS-TOOLCHAIN with default values
desirable for building Bitcoin Core release binaries."
  (make-cross-toolchain target
                   base-gcc-for-libc
                   base-kernel-headers
                   base-libc
                   base-gcc))

(packages->manifest
 (list ;; The Basics
       bash-minimal
       which
       coreutils
       util-linux
       ;; File(system) inspection
       file
       grep
       diffutils
       findutils
       ;; File transformation
       patch
       gawk
       sed
       ;; Compression and archiving
       tar
       bzip2
       gzip
       xz
       zlib
       ;; Build tools
       gnu-make
       libtool
       autoconf
       automake
       pkg-config
       ;; Scripting
       perl
       python-3.7
       ;; Native gcc 9 toolchain targeting glibc 2.27
       (make-gcc-toolchain gcc-9 glibc-2.27)
       ;; Cross gcc 9 toolchains targeting glibc 2.27
       (make-bitcoin-cross-toolchain "i686-linux-gnu")
       (make-bitcoin-cross-toolchain "x86_64-linux-gnu")
       (make-bitcoin-cross-toolchain "aarch64-linux-gnu")
       (make-bitcoin-cross-toolchain "arm-linux-gnueabihf")
       ;; The glibc 2.27 for riscv64 needs gcc 7 to successfully build (see:
       ;; https://www.gnu.org/software/gcc/gcc-7/changes.html#riscv). The final
       ;; toolchain is still a gcc 9 toolchain targeting glibc 2.27.
       (make-bitcoin-cross-toolchain "riscv64-linux-gnu"
                                     #:base-gcc-for-libc gcc-7)))
