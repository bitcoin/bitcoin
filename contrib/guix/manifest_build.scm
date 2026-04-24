(use-modules (gnu packages)
             ((gnu packages bash) #:select (bash-minimal))
             (gnu packages bison)
             ((gnu packages cmake) #:select (cmake-minimal))
             (gnu packages commencement)
             (gnu packages compression)
             (gnu packages cross-base)
             (gnu packages gawk)
             (gnu packages gcc)
             ((gnu packages installers) #:select (nsis-x86_64))
             ((gnu packages linux) #:select (linux-libre-headers-6.1))
             (gnu packages llvm)
             (gnu packages mingw)
             (gnu packages ninja)
             (gnu packages pkg-config)
             ((gnu packages python) #:select (python-minimal))
             ((gnu packages python-xyz) #:select (python-lief))
             ((gnu packages version-control) #:select (git-minimal))
             (guix build-system trivial)
             (guix download)
             (guix gexp)
             (guix git-download)
             ((guix licenses) #:prefix license:)
             (guix packages)
             ((guix utils) #:select (substitute-keyword-arguments)))

(define-syntax-rule (search-our-patches file-name ...)
  "Return the list of absolute file names corresponding to each
FILE-NAME found in ./patches relative to the current file."
  (parameterize
      ((%patch-path (list (string-append (dirname (current-filename)) "/patches"))))
    (list (search-patch file-name) ...)))

(define building-on (string-append "--build=" (list-ref (string-split (%current-system) #\-) 0) "-guix-linux-gnu"))

(define (base-binutils target)
  (package
    (inherit (cross-binutils target)) ;; 2.44
    (version "2.46.0")
    (source (origin
              (method url-fetch)
              (uri (string-append "mirror://gnu/binutils/binutils-"
                          version ".tar.bz2"))
              (sha256
               (base32
                "04nd9vl7c1pxjbc9wh3ckddzhz5g82xyjqq9y9kf171a59im4c8g"))))
    (arguments
      (substitute-keyword-arguments (package-arguments (cross-binutils target))
        ((#:configure-flags flags)
          #~(append #$flags
            (list "--enable-gprofng=no")))))
    (native-inputs
      (modify-inputs
        (package-native-inputs (cross-binutils target))
        (delete "bison")))
  )
)

(define (make-cross-toolchain target
                              base-gcc-for-libc
                              base-kernel-headers
                              base-libc
                              base-gcc)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (base-binutils target))
         ;; 1. Build a cross-compiling gcc without targeting any libc, derived
         ;; from BASE-GCC-FOR-LIBC
         (xgcc-sans-libc (cross-gcc target
                                    #:xgcc base-gcc-for-libc
                                    #:xbinutils xbinutils))
         ;; 2. Build cross-compiled kernel headers with XGCC-SANS-LIBC, derived
         ;; from BASE-KERNEL-HEADERS
         (xkernel (cross-kernel-headers target
                                        #:linux-headers base-kernel-headers
                                        #:xgcc xgcc-sans-libc
                                        #:xbinutils xbinutils))
         ;; 3. Build a cross-compiled libc with XGCC-SANS-LIBC and XKERNEL,
         ;; derived from BASE-LIBC
         (xlibc (cross-libc target
                            #:libc base-libc
                            #:xgcc xgcc-sans-libc
                            #:xbinutils xbinutils
                            #:xheaders xkernel))
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
        (list xbinutils
              xlibc
              xgcc
              `(,xlibc "static")
              `(,xgcc "lib")))
      (synopsis (string-append "Complete GCC tool chain for " target))
      (description (string-append "This package provides a complete GCC tool
chain for " target " development."))
      (home-page (package-home-page xgcc))
      (license (package-license xgcc)))))

(define base-gcc
  (package-with-extra-patches gcc-14
    (search-our-patches "gcc-remap-guix-store.patch" "gcc-ssa-generation.patch")))

(define base-linux-kernel-headers linux-libre-headers-6.1)

(define* (make-bitcoin-cross-toolchain target
                                       #:key
                                       (base-gcc-for-libc linux-base-gcc)
                                       (base-kernel-headers base-linux-kernel-headers)
                                       (base-libc glibc-2.31)
                                       (base-gcc linux-base-gcc))
  "Convenience wrapper around MAKE-CROSS-TOOLCHAIN with default values
desirable for building Bitcoin Core release binaries."
  (make-cross-toolchain target
                        base-gcc-for-libc
                        base-kernel-headers
                        base-libc
                        base-gcc))

(define (binutils-mingw-patches binutils)
  (package-with-extra-patches binutils
    (search-our-patches "binutils-unaligned-default.patch")))

(define (winpthreads-patches mingw-w64-x86_64-winpthreads)
  (package-with-extra-patches mingw-w64-x86_64-winpthreads
    (search-our-patches "winpthreads-remap-guix-store.patch")))

(define (make-mingw-pthreads-cross-toolchain target)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (binutils-mingw-patches (base-binutils target)))
         (machine (substring target 0 (string-index target #\-)))
         (pthreads-xlibc (winpthreads-patches (make-mingw-w64 machine
                                         #:xgcc (cross-gcc target #:xgcc base-gcc)
                                         #:with-winpthreads? #t)))
         (pthreads-xgcc (cross-gcc target
                                    #:xgcc mingw-w64-base-gcc
                                    #:xbinutils xbinutils
                                    #:libc pthreads-xlibc)))
    ;; Define a meta-package that propagates the resulting XBINUTILS, XLIBC, and
    ;; XGCC
    (package
      (name (string-append target "-posix-toolchain"))
      (version (package-version pthreads-xgcc))
      (source #f)
      (build-system trivial-build-system)
      (arguments '(#:builder (begin (mkdir %output) #t)))
      (propagated-inputs
        (list xbinutils
              pthreads-xlibc
              pthreads-xgcc
              `(,pthreads-xgcc "lib")))
      (synopsis (string-append "Complete GCC tool chain for " target))
      (description (string-append "This package provides a complete GCC tool
chain for " target " development."))
      (home-page (package-home-page pthreads-xgcc))
      (license (package-license pthreads-xgcc)))))

(define-public mingw-w64-base-gcc
  (package
    (inherit base-gcc)
    (arguments
      (substitute-keyword-arguments (package-arguments base-gcc)
        ((#:configure-flags flags)
          `(append ,flags
            ;; https://gcc.gnu.org/install/configure.html
            (list "--enable-threads=posix",
                  "--enable-default-ssp=yes",
                  "--enable-host-bind-now=yes",
                  "--disable-gcov",
                  "--disable-libgomp",
                  building-on)))))))

(define-public linux-base-gcc
  (package
    (inherit base-gcc)
    (arguments
      (substitute-keyword-arguments (package-arguments base-gcc)
        ((#:configure-flags flags)
          `(append ,flags
            ;; https://gcc.gnu.org/install/configure.html
            (list "--enable-initfini-array=yes",
                  "--enable-default-ssp=yes",
                  "--enable-default-pie=yes",
                  "--enable-host-bind-now=yes",
                  "--enable-standard-branch-protection=yes",
                  "--enable-cet=yes",
                  "--enable-gprofng=no",
                  "--disable-gcov",
                  "--disable-libgomp",
                  "--disable-libquadmath",
                  "--disable-libsanitizer",
                  building-on)))
        ((#:phases phases)
          `(modify-phases ,phases
            ;; Given a XGCC package, return a modified package that replace each instance of
            ;; -rpath in the default system spec that's inserted by Guix with -rpath-link
            (add-after 'pre-configure 'replace-rpath-with-rpath-link
             (lambda _
               (substitute* (cons "gcc/config/rs6000/sysv4.h"
                                  (find-files "gcc/config"
                                              "^gnu-user.*\\.h$"))
                 (("-rpath=") "-rpath-link="))
               #t))))))))

(define-public glibc-2.31
  (let ((commit "28eb5caf895ced5d895cb02757e109004a2d33e5"))
  (package
    (inherit glibc) ;; 2.39
    (version "2.31")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://sourceware.org/git/glibc.git")
                    (commit commit)))
              (file-name (git-file-name "glibc" commit))
              (sha256
               (base32
                "07arjrc1smqy8wrhg38apr1s9ji7xv1rpzdapk4k2ps2n07irp58"))
              (patches (search-our-patches "glibc-guix-prefix.patch"
                                           "glibc-riscv-jumptarget.patch"))))
    (arguments
      (substitute-keyword-arguments (package-arguments glibc)
        ((#:configure-flags flags)
          `(append ,flags
            ;; https://www.gnu.org/software/libc/manual/html_node/Configuring-and-compiling.html
            (list "--enable-stack-protector=all",
                  "--enable-cet",
                  "--enable-bind-now",
                  "--disable-werror",
                  "--disable-timezone-tools",
                  "--disable-profile",
                  building-on)))
    ((#:phases phases)
        `(modify-phases ,phases
           (add-before 'configure 'set-etc-rpc-installation-directory
             (lambda* (#:key outputs #:allow-other-keys)
               ;; Install the rpc data base file under `$out/etc/rpc'.
               ;; Otherwise build will fail with "Permission denied."
               ;; Can be removed when we are building 2.32 or later.
               (let ((out (assoc-ref outputs "out")))
                 (substitute* "sunrpc/Makefile"
                   (("^\\$\\(inst_sysconfdir\\)/rpc(.*)$" _ suffix)
                    (string-append out "/etc/rpc" suffix "\n"))
                   (("^install-others =.*$")
                    (string-append "install-others = " out "/etc/rpc\n")))))))))))))

(packages->manifest
 (append
  (list ;; The Basics
        bash-minimal
        which
        coreutils-minimal
        ;; File(system) inspection
        grep
        diffutils
        findutils
        ;; File transformation
        patch
        gawk
        sed
        ;; Compression and archiving
        tar
        gzip
        xz
        ;; Build tools
        gcc-toolchain-14
        cmake-minimal
        gnu-make
        ninja
        ;; Scripting
        python-minimal ;; (3.11)
        ;; Git
        git-minimal
        ;; Tests
        python-lief)
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           (list zip
                 (make-mingw-pthreads-cross-toolchain "x86_64-w64-mingw32")
                 nsis-x86_64))
          ((string-contains target "-linux-")
           (list bison
                 pkg-config
                 (list gcc-toolchain-14 "static")
                 (make-bitcoin-cross-toolchain target)))
          ((string-contains target "darwin")
           (list clang-toolchain-19
                 lld-19
                 (make-lld-wrapper lld-19 #:lld-as-ld? #t)
                 zip))
          (else '())))))
