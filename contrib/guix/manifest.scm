(use-modules (gnu packages)
             (gnu packages autotools)
             ((gnu packages bash) #:select (bash-minimal))
             (gnu packages bison)
             ((gnu packages certs) #:select (nss-certs))
             ((gnu packages cmake) #:select (cmake-minimal))
             (gnu packages commencement)
             (gnu packages compression)
             (gnu packages cross-base)
             (gnu packages file)
             (gnu packages gawk)
             (gnu packages gcc)
             ((gnu packages installers) #:select (nsis-x86_64))
             ((gnu packages linux) #:select (linux-libre-headers-6.1 util-linux))
             (gnu packages llvm)
             (gnu packages mingw)
             (gnu packages moreutils)
             (gnu packages pkg-config)
             ((gnu packages python) #:select (python-minimal))
             ((gnu packages python-build) #:select (python-tomli))
             ((gnu packages python-crypto) #:select (python-asn1crypto))
             ((gnu packages tls) #:select (openssl))
             ((gnu packages version-control) #:select (git-minimal))
             (guix build-system cmake)
             (guix build-system gnu)
             (guix build-system python)
             (guix build-system trivial)
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

(define base-gcc gcc-12)
(define base-linux-kernel-headers linux-libre-headers-6.1)

(define* (make-bitcoin-cross-toolchain target
                                       #:key
                                       (base-gcc-for-libc linux-base-gcc)
                                       (base-kernel-headers base-linux-kernel-headers)
                                       (base-libc glibc-2.33)
                                       (base-gcc linux-base-gcc))
  "Convenience wrapper around MAKE-CROSS-TOOLCHAIN with default values
desirable for building Bitcoin Core release binaries."
  (make-cross-toolchain target
                        base-gcc-for-libc
                        base-kernel-headers
                        base-libc
                        base-gcc))

(define (gcc-mingw-patches gcc)
  (package-with-extra-patches gcc
    (search-our-patches "gcc-remap-guix-store.patch")))

(define (binutils-mingw-patches binutils)
  (package-with-extra-patches binutils
    (search-our-patches "binutils-unaligned-default.patch")))

(define (make-mingw-pthreads-cross-toolchain target)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (binutils-mingw-patches (cross-binutils target)))
         (pthreads-xlibc mingw-w64-x86_64-winpthreads)
         (pthreads-xgcc (cross-gcc target
                                    #:xgcc (gcc-mingw-patches mingw-w64-base-gcc)
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

;; While LIEF is packaged in Guix, we maintain our own package,
;; to simplify building, and more easily apply updates.
;; Moreover, the Guix's package uses cmake, which caused build
;; failure; see https://github.com/bitcoin/bitcoin/pull/27296.
(define-public python-lief
  (package
    (name "python-lief")
    (version "0.13.2")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://github.com/lief-project/LIEF")
                    (commit version)))
              (file-name (git-file-name name version))
              (modules '((guix build utils)))
              (snippet
               '(begin
                  ;; Configure build for Python bindings.
                  (substitute* "api/python/config-default.toml"
                    (("(ninja         = )true" all m)
                     (string-append m "false"))
                    (("(parallel-jobs = )0" all m)
                     (string-append m (number->string (parallel-job-count)))))))
              (sha256
               (base32
                "0y48x358ppig5xp97ahcphfipx7cg9chldj2q5zrmn610fmi4zll"))))
    (build-system python-build-system)
    (native-inputs (list cmake-minimal python-tomli))
    (arguments
     (list
      #:tests? #f                  ;needs network
      #:phases #~(modify-phases %standard-phases
                   (add-before 'build 'change-directory
                     (lambda _
                       (chdir "api/python")))
                   (replace 'build
                     (lambda _
                       (invoke "python" "setup.py" "build"))))))
    (home-page "https://github.com/lief-project/LIEF")
    (synopsis "Library to instrument executable formats")
    (description
     "@code{python-lief} is a cross platform library which can parse, modify
and abstract ELF, PE and MachO formats.")
    (license license:asl2.0)))

(define osslsigncode
  (package
    (name "osslsigncode")
    (version "2.5")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://github.com/mtrojnar/osslsigncode")
                    (commit version)))
              (sha256
               (base32
                "1j47vwq4caxfv0xw68kw5yh00qcpbd56d7rq6c483ma3y7s96yyz"))))
    (build-system cmake-build-system)
    (inputs (list openssl))
    (home-page "https://github.com/mtrojnar/osslsigncode")
    (synopsis "Authenticode signing and timestamping tool")
    (description "osslsigncode is a small tool that implements part of the
functionality of the Microsoft tool signtool.exe - more exactly the Authenticode
signing and timestamping. But osslsigncode is based on OpenSSL and cURL, and
thus should be able to compile on most platforms where these exist.")
    (license license:gpl3+))) ; license is with openssl exception

(define-public python-elfesteem
  (let ((commit "2eb1e5384ff7a220fd1afacd4a0170acff54fe56"))
    (package
      (name "python-elfesteem")
      (version (git-version "0.1" "1" commit))
      (source
       (origin
         (method git-fetch)
         (uri (git-reference
               (url "https://github.com/LRGH/elfesteem")
               (commit commit)))
         (file-name (git-file-name name commit))
         (sha256
          (base32
           "07x6p8clh11z8s1n2kdxrqwqm2almgc5qpkcr9ckb6y5ivjdr5r6"))))
      (build-system python-build-system)
      ;; There are no tests, but attempting to run python setup.py test leads to
      ;; PYTHONPATH problems, just disable the test
      (arguments '(#:tests? #f))
      (home-page "https://github.com/LRGH/elfesteem")
      (synopsis "ELF/PE/Mach-O parsing library")
      (description "elfesteem parses ELF, PE and Mach-O files.")
      (license license:lgpl2.1))))

(define-public python-oscrypto
  (package
    (name "python-oscrypto")
    (version "1.3.0")
    (source
     (origin
       (method git-fetch)
       (uri (git-reference
             (url "https://github.com/wbond/oscrypto")
             (commit version)))
       (file-name (git-file-name name version))
       (sha256
        (base32
         "1v5wkmzcyiqy39db8j2dvkdrv2nlsc48556h73x4dzjwd6kg4q0a"))
       (patches (search-our-patches "oscrypto-hard-code-openssl.patch"))))
    (build-system python-build-system)
    (native-search-paths
     (list (search-path-specification
            (variable "SSL_CERT_FILE")
            (file-type 'regular)
            (separator #f)                ;single entry
            (files '("etc/ssl/certs/ca-certificates.crt")))))

    (propagated-inputs
      (list python-asn1crypto openssl))
    (arguments
     `(#:phases
       (modify-phases %standard-phases
         (add-after 'unpack 'hard-code-path-to-libscrypt
           (lambda* (#:key inputs #:allow-other-keys)
             (let ((openssl (assoc-ref inputs "openssl")))
               (substitute* "oscrypto/__init__.py"
                 (("@GUIX_OSCRYPTO_USE_OPENSSL@")
                  (string-append openssl "/lib/libcrypto.so" "," openssl "/lib/libssl.so")))
               #t)))
         (add-after 'unpack 'disable-broken-tests
           (lambda _
             ;; This test is broken as there is no keyboard interrupt.
             (substitute* "tests/test_trust_list.py"
               (("^(.*)class TrustListTests" line indent)
                (string-append indent
                               "@unittest.skip(\"Disabled by Guix\")\n"
                               line)))
             (substitute* "tests/test_tls.py"
               (("^(.*)class TLSTests" line indent)
                (string-append indent
                               "@unittest.skip(\"Disabled by Guix\")\n"
                               line)))
             #t))
         (replace 'check
           (lambda _
             (invoke "python" "run.py" "tests")
             #t)))))
    (home-page "https://github.com/wbond/oscrypto")
    (synopsis "Compiler-free Python crypto library backed by the OS")
    (description "oscrypto is a compilation-free, always up-to-date encryption library for Python.")
    (license license:expat)))

(define-public python-oscryptotests
  (package (inherit python-oscrypto)
    (name "python-oscryptotests")
    (propagated-inputs
      (list python-oscrypto))
    (arguments
     `(#:tests? #f
       #:phases
       (modify-phases %standard-phases
         (add-after 'unpack 'hard-code-path-to-libscrypt
           (lambda* (#:key inputs #:allow-other-keys)
             (chdir "tests")
             #t)))))))

(define-public python-certvalidator
  (let ((commit "a145bf25eb75a9f014b3e7678826132efbba6213"))
    (package
      (name "python-certvalidator")
      (version (git-version "0.1" "1" commit))
      (source
       (origin
         (method git-fetch)
         (uri (git-reference
               (url "https://github.com/achow101/certvalidator")
               (commit commit)))
         (file-name (git-file-name name commit))
         (sha256
          (base32
           "1qw2k7xis53179lpqdqyylbcmp76lj7sagp883wmxg5i7chhc96k"))))
      (build-system python-build-system)
      (propagated-inputs
        (list python-asn1crypto
              python-oscrypto
              python-oscryptotests)) ;; certvalidator tests import oscryptotests
      (arguments
       `(#:phases
         (modify-phases %standard-phases
           (add-after 'unpack 'disable-broken-tests
             (lambda _
               (substitute* "tests/test_certificate_validator.py"
                 (("^(.*)class CertificateValidatorTests" line indent)
                  (string-append indent
                                 "@unittest.skip(\"Disabled by Guix\")\n"
                                 line)))
               (substitute* "tests/test_crl_client.py"
                 (("^(.*)def test_fetch_crl" line indent)
                  (string-append indent
                                 "@unittest.skip(\"Disabled by Guix\")\n"
                                 line)))
               (substitute* "tests/test_ocsp_client.py"
                 (("^(.*)def test_fetch_ocsp" line indent)
                  (string-append indent
                                 "@unittest.skip(\"Disabled by Guix\")\n"
                                 line)))
               (substitute* "tests/test_registry.py"
                 (("^(.*)def test_build_paths" line indent)
                  (string-append indent
                                 "@unittest.skip(\"Disabled by Guix\")\n"
                                 line)))
               (substitute* "tests/test_validate.py"
                 (("^(.*)def test_revocation_mode_hard" line indent)
                  (string-append indent
                                 "@unittest.skip(\"Disabled by Guix\")\n"
                                 line)))
               (substitute* "tests/test_validate.py"
                 (("^(.*)def test_revocation_mode_soft" line indent)
                  (string-append indent
                                 "@unittest.skip(\"Disabled by Guix\")\n"
                                 line)))
               #t))
           (replace 'check
             (lambda _
               (invoke "python" "run.py" "tests")
               #t)))))
      (home-page "https://github.com/wbond/certvalidator")
      (synopsis "Python library for validating X.509 certificates and paths")
      (description "certvalidator is a Python library for validating X.509
certificates or paths. Supports various options, including: validation at a
specific moment in time, whitelisting and revocation checks.")
      (license license:expat))))

(define-public python-signapple
  (let ((commit "62155712e7417aba07565c9780a80e452823ae6a"))
    (package
      (name "python-signapple")
      (version (git-version "0.1" "1" commit))
      (source
       (origin
         (method git-fetch)
         (uri (git-reference
               (url "https://github.com/achow101/signapple")
               (commit commit)))
         (file-name (git-file-name name commit))
         (sha256
          (base32
           "1nm6rm4h4m7kbq729si4cm8rzild62mk4ni8xr5zja7l33fhv3gb"))))
      (build-system python-build-system)
      (propagated-inputs
        (list python-asn1crypto
              python-oscrypto
              python-certvalidator
              python-elfesteem))
      ;; There are no tests, but attempting to run python setup.py test leads to
      ;; problems, just disable the test
      (arguments '(#:tests? #f))
      (home-page "https://github.com/achow101/signapple")
      (synopsis "Mach-O binary signature tool")
      (description "signapple is a Python tool for creating, verifying, and
inspecting signatures in Mach-O binaries.")
      (license license:expat))))

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
                  "--enable-standard-branch-protection=yes",
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

(define-public glibc-2.33
  (let ((commit "d0916db23313266770c865091cc3d4ae69871e60"))
  (package
    (inherit glibc) ;; 2.35
    (version "2.33")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://sourceware.org/git/glibc.git")
                    (commit commit)))
              (file-name (git-file-name "glibc" commit))
              (sha256
               (base32
                "1by2vlwbq10fjs1l8qm0xpkm90nwc3awajjklqiz6dj60l68cj9q"))
              (patches (search-our-patches "glibc-guix-prefix.patch"))))
    (arguments
      (substitute-keyword-arguments (package-arguments glibc)
        ((#:configure-flags flags)
          `(append ,flags
            ;; https://www.gnu.org/software/libc/manual/html_node/Configuring-and-compiling.html
            (list "--enable-stack-protector=all",
                  "--enable-bind-now",
                  "--disable-werror",
                  building-on))))))))

(packages->manifest
 (append
  (list ;; The Basics
        bash-minimal
        which
        coreutils-minimal
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
        moreutils
        ;; Compression and archiving
        tar
        gzip
        xz
        ;; Build tools
        cmake-minimal
        gnu-make
        libtool
        autoconf-2.71
        automake
        pkg-config
        bison
        ;; Scripting
        python-minimal ;; (3.10)
        ;; Git
        git-minimal
        ;; Tests
        python-lief)
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           (list ;; Native GCC 12 toolchain
                 gcc-toolchain-12
                 zip
                 (make-mingw-pthreads-cross-toolchain "x86_64-w64-mingw32")
                 nsis-x86_64
                 nss-certs
                 osslsigncode))
          ((string-contains target "-linux-")
           (list ;; Native GCC 12 toolchain
                 gcc-toolchain-12
                 (list gcc-toolchain-12 "static")
                 (make-bitcoin-cross-toolchain target)))
          ((string-contains target "darwin")
           (list ;; Native GCC 11 toolchain
                 gcc-toolchain-11
                 clang-toolchain-18
                 lld-18
                 (make-lld-wrapper lld-18 #:lld-as-ld? #t)
                 python-signapple
                 zip))
          (else '())))))
