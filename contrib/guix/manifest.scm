(use-modules (gnu)
             (gnu packages)
             (gnu packages autotools)
             (gnu packages base)
             (gnu packages bash)
             (gnu packages bison)
             (gnu packages cdrom)
             (gnu packages check)
             (gnu packages cmake)
             (gnu packages commencement)
             (gnu packages compression)
             (gnu packages cross-base)
             (gnu packages file)
             (gnu packages gawk)
             (gnu packages gcc)
             (gnu packages gnome)
             (gnu packages image)
             (gnu packages imagemagick)
             (gnu packages installers)
             (gnu packages linux)
             (gnu packages llvm)
             (gnu packages mingw)
             (gnu packages perl)
             (gnu packages pkg-config)
             (gnu packages python)
             (gnu packages shells)
             (gnu packages version-control)
             (guix build-system font)
             (guix build-system gnu)
             (guix build-system python)
             (guix build-system trivial)
             (guix download)
             (guix gexp)
             (guix git-download)
             ((guix licenses) #:prefix license:)
             (guix packages)
             (guix profiles)
             (guix utils))

(define-syntax-rule (search-our-patches file-name ...)
  "Return the list of absolute file names corresponding to each
FILE-NAME found in ./patches relative to the current file."
  (parameterize
      ((%patch-path (list (string-append (dirname (current-filename)) "/patches"))))
    (list (search-patch file-name) ...)))

(define (make-ssp-fixed-gcc xgcc)
  "Given a XGCC package, return a modified package that uses the SSP function
from glibc instead of from libssp.so. Our `symbol-check' script will complain if
we link against libssp.so, and thus will ensure that this works properly.

Taken from:
http://www.linuxfromscratch.org/hlfs/view/development/chapter05/gcc-pass1.html"
  (package
   (inherit xgcc)
   (arguments
    (substitute-keyword-arguments (package-arguments xgcc)
      ((#:make-flags flags)
       `(cons "gcc_cv_libc_provides_ssp=yes" ,flags))))))

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

(define (make-binutils-with-mingw-w64-disable-flags xbinutils)
  (package-with-extra-patches xbinutils
    (search-our-patches "binutils-mingw-w64-disable-flags.patch")))

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
         ("gcc" ,xgcc)
         ("gcc-lib" ,xgcc "lib")))
      (synopsis (string-append "Complete GCC tool chain for " target))
      (description (string-append "This package provides a complete GCC tool
chain for " target " development."))
      (home-page (package-home-page xgcc))
      (license (package-license xgcc)))))

(define* (make-bitcoin-cross-toolchain target
                                  #:key
                                  (base-gcc-for-libc gcc-7)
                                  (base-kernel-headers linux-libre-headers-5.4)
                                  (base-libc glibc)  ; glibc 2.31
                                  (base-gcc (make-gcc-rpath-link gcc-8)))
  "Convenience wrapper around MAKE-CROSS-TOOLCHAIN with default values
desirable for building Bitcoin Core release binaries."
  (make-cross-toolchain target
                   base-gcc-for-libc
                   base-kernel-headers
                   base-libc
                   base-gcc))

(define (make-gcc-with-pthreads gcc)
  (package-with-extra-configure-variable gcc "--enable-threads" "posix"))

(define (make-mingw-pthreads-cross-toolchain target)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (make-binutils-with-mingw-w64-disable-flags (cross-binutils target)))
         (pthreads-xlibc mingw-w64-x86_64-winpthreads)
         (pthreads-xgcc (make-gcc-with-pthreads
                         (cross-gcc target
                                    #:xgcc (make-ssp-fixed-gcc gcc-8)
                                    #:xbinutils xbinutils
                                    #:libc pthreads-xlibc))))
    ;; Define a meta-package that propagates the resulting XBINUTILS, XLIBC, and
    ;; XGCC
    (package
      (name (string-append target "-posix-toolchain"))
      (version (package-version pthreads-xgcc))
      (source #f)
      (build-system trivial-build-system)
      (arguments '(#:builder (begin (mkdir %output) #t)))
      (propagated-inputs
       `(("binutils" ,xbinutils)
         ("libc" ,pthreads-xlibc)
         ("gcc" ,pthreads-xgcc)
         ("gcc-lib" ,pthreads-xgcc "lib")))
      (synopsis (string-append "Complete GCC tool chain for " target))
      (description (string-append "This package provides a complete GCC tool
chain for " target " development."))
      (home-page (package-home-page pthreads-xgcc))
      (license (package-license pthreads-xgcc)))))

(define (make-nsis-with-sde-support base-nsis)
  (package-with-extra-patches base-nsis
    (search-our-patches "nsis-SConstruct-sde-support.patch")))

(define-public font-tuffy
  (package
   (name "font-tuffy")
   (version "20120614")
   (source
    (origin
     (method url-fetch)
     (uri (string-append "http://tulrich.com/fonts/tuffy-" version ".tar.gz"))
     (file-name (string-append name "-" version ".tar.gz"))
     (sha256
      (base32
       "02vf72bgrp30vrbfhxjw82s115z27dwfgnmmzfb0n9wfhxxfpyf6"))))
   (build-system font-build-system)
   (home-page "http://tulrich.com/fonts/")
   (synopsis "The Tuffy Truetype Font Family")
   (description
    "Thatcher Ulrich's first outline font design. He started with the goal of producing a neutral, readable sans-serif text font. There are lots of \"expressive\" fonts out there, but he wanted to start with something very plain and clean, something he might want to actually use. ")
   (license license:public-domain)))

(define-public lief
  (package
   (name "python-lief")
   (version "0.11.4")
   (source
    (origin
     (method git-fetch)
     (uri (git-reference
           (url "https://github.com/lief-project/LIEF.git")
           (commit version)))
     (file-name (git-file-name name version))
     (sha256
      (base32
       "0h4kcwr9z478almjqhmils8imfpflzk0r7d05g4xbkdyknn162qf"))))
   (build-system python-build-system)
   (native-inputs
    `(("cmake" ,cmake)))
   (home-page "https://github.com/lief-project/LIEF")
   (synopsis "Library to Instrument Executable Formats")
   (description "Python library to to provide a cross platform library which can
parse, modify and abstract ELF, PE and MachO formats.")
   (license license:asl2.0)))

(packages->manifest
 (append
  (list ;; The Basics
        bash
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
        (list zlib "static")
        ;; Build tools
        gnu-make
        libtool
        autoconf
        automake
        pkg-config
        bison
        ;; Scripting
        perl
        python-3
        ;; Git
        git
        ;; Tests
        lief
        ;; Native gcc 7 toolchain
        gcc-toolchain-7
        (list gcc-toolchain-7 "static"))
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           ;; Windows
           (list zip
                 (make-mingw-pthreads-cross-toolchain "x86_64-w64-mingw32")
                 (make-nsis-with-sde-support nsis-x86_64)))
          ((string-contains target "-linux-")
           (list (make-bitcoin-cross-toolchain target)))
          ((string-contains target "darwin")
           (list clang-toolchain-10 binutils imagemagick libtiff librsvg font-tuffy cmake xorriso))
          (else '())))))
