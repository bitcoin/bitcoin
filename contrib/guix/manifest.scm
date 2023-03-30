(use-modules (gnu)
             (gnu packages)
             (gnu packages autotools)
             (gnu packages base)
             (gnu packages bash)
             (gnu packages bison)
             (gnu packages certs)
             (gnu packages cdrom)
             (gnu packages check)
             (gnu packages cmake)
             (gnu packages commencement)
             (gnu packages compression)
             (gnu packages cross-base)
             (gnu packages curl)
             (gnu packages file)
             (gnu packages gawk)
             (gnu packages gcc)
             (gnu packages gnome)
             (gnu packages installers)
             (gnu packages linux)
             (gnu packages llvm)
             (gnu packages mingw)
             (gnu packages moreutils)
             (gnu packages pkg-config)
             (gnu packages python)
             (gnu packages python-crypto)
             (gnu packages python-web)
             (gnu packages shells)
             (gnu packages tls)
             (gnu packages version-control)
             (guix build-system cmake)
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

(define building-on (string-append (list-ref (string-split (%current-system) #\-) 0) "-guix-linux-gnu"))

(define (explicit-cross-configure package)
  (package-with-extra-configure-variable package "--build" building-on))

(define (make-cross-toolchain target
                              base-gcc-for-libc
                              base-kernel-headers
                              base-libc
                              base-gcc)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (cross-binutils target))
         ;; 1. Build a cross-compiling gcc without targeting any libc, derived
         ;; from BASE-GCC-FOR-LIBC
         (xgcc-sans-libc (explicit-cross-configure (cross-gcc target
                                                              #:xgcc base-gcc-for-libc
                                                              #:xbinutils xbinutils)))
         ;; 2. Build cross-compiled kernel headers with XGCC-SANS-LIBC, derived
         ;; from BASE-KERNEL-HEADERS
         (xkernel (cross-kernel-headers target
                                        base-kernel-headers
                                        xgcc-sans-libc
                                        xbinutils))
         ;; 3. Build a cross-compiled libc with XGCC-SANS-LIBC and XKERNEL,
         ;; derived from BASE-LIBC
         (xlibc (explicit-cross-configure (cross-libc target
                                                      base-libc
                                                      xgcc-sans-libc
                                                      xbinutils
                                                      xkernel)))
         ;; 4. Build a cross-compiling gcc targeting XLIBC, derived from
         ;; BASE-GCC
         (xgcc (explicit-cross-configure (cross-gcc target
                                                    #:xgcc base-gcc
                                                    #:xbinutils xbinutils
                                                    #:libc xlibc))))
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

(define base-gcc gcc-10)
(define base-linux-kernel-headers linux-libre-headers-5.15)

;; https://gcc.gnu.org/install/configure.html
(define (hardened-gcc gcc)
  (package-with-extra-configure-variable (
    package-with-extra-configure-variable (
      package-with-extra-configure-variable gcc
      "--enable-initfini-array" "yes")
      "--enable-default-ssp" "yes")
      "--enable-default-pie" "yes"))

(define* (make-bitcoin-cross-toolchain target
                                       #:key
                                       (base-gcc-for-libc base-gcc)
                                       (base-kernel-headers base-linux-kernel-headers)
                                       (base-libc (hardened-glibc glibc-2.27))
                                       (base-gcc (make-gcc-rpath-link (hardened-gcc base-gcc))))
  "Convenience wrapper around MAKE-CROSS-TOOLCHAIN with default values
desirable for building Bitcoin Core release binaries."
  (make-cross-toolchain target
                        base-gcc-for-libc
                        base-kernel-headers
                        base-libc
                        base-gcc))

(define (make-gcc-with-pthreads gcc)
  (package-with-extra-configure-variable
    (package-with-extra-patches gcc
      (search-our-patches "gcc-10-remap-guix-store.patch"))
    "--enable-threads" "posix"))

(define (make-mingw-w64-cross-gcc cross-gcc)
  (package-with-extra-patches cross-gcc
    (search-our-patches "vmov-alignment.patch"
                        "gcc-broken-longjmp.patch")))

(define (make-mingw-pthreads-cross-toolchain target)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (cross-binutils target))
         (pthreads-xlibc mingw-w64-x86_64-winpthreads)
         (pthreads-xgcc (make-gcc-with-pthreads
                         (cross-gcc target
                                    #:xgcc (make-ssp-fixed-gcc (make-mingw-w64-cross-gcc base-gcc))
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

(define (make-nsis-for-gcc-10 base-nsis)
  (package-with-extra-patches base-nsis
    (search-our-patches "nsis-gcc-10-memmove.patch"
                        "nsis-disable-installer-reloc.patch")))

(define (fix-ppc64-nx-default lief)
  (package-with-extra-patches lief
    (search-our-patches "lief-fix-ppc64-nx-default.patch")))

;; Our python-lief package can be removed once we are using
;; guix 83bfdb409787cb2737e68b093a319b247b7858e6 or later.
;; Note we currently use cmake-minimal.
(define-public python-lief
  (package
    (name "python-lief")
    (version "0.12.3")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://github.com/lief-project/LIEF")
                    (commit version)))
              (file-name (git-file-name name version))
              (sha256
               (base32
                "11i6hqmcjh56y554kqhl61698n9v66j2qk1c1g63mv2w07h2z661"))))
    (build-system python-build-system)
    (native-inputs (list cmake-minimal))
    (arguments
     (list
      #:tests? #f                  ;needs network
      #:phases #~(modify-phases %standard-phases
                   (replace 'build
                     (lambda _
                       (invoke
                        "python" "setup.py" "--sdk" "build"
                        (string-append
                         "-j" (number->string (parallel-job-count)))))))))
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
              (method url-fetch)
              (uri (string-append "https://github.com/mtrojnar/"
                                  name "/archive/" version ".tar.gz"))
              (sha256
               (base32
                "03by9706gg0an6dn48pljx38vcb76ziv11bgm8ilwsf293x2k4hv"))))
    (build-system cmake-build-system)
    (inputs
     `(("openssl", openssl)))
    (arguments
     '(#:configure-flags
        (list "-DCMAKE_DISABLE_FIND_PACKAGE_CURL=TRUE")))
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
    (version "1.2.1")
    (source
     (origin
       (method git-fetch)
       (uri (git-reference
             (url "https://github.com/wbond/oscrypto")
             (commit version)))
       (file-name (git-file-name name version))
       (sha256
        (base32
         "1d4d8s4z340qhvb3g5m5v3436y3a71yc26wk4749q64m09kxqc3l"))
       (patches (search-our-patches "oscrypto-hard-code-openssl.patch"))))
    (build-system python-build-system)
    (native-search-paths
     (list (search-path-specification
            (variable "SSL_CERT_FILE")
            (file-type 'regular)
            (separator #f)                ;single entry
            (files '("etc/ssl/certs/ca-certificates.crt")))))

    (propagated-inputs
     `(("python-asn1crypto" ,python-asn1crypto)
       ("openssl" ,openssl)))
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
      `(("python-oscrypto" ,python-oscrypto)))
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
       `(("python-asn1crypto" ,python-asn1crypto)
         ("python-oscrypto" ,python-oscrypto)
         ("python-oscryptotests", python-oscryptotests))) ;; certvalidator tests import oscryptotests
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

(define-public python-altgraph
  (package
    (name "python-altgraph")
    (version "0.17")
    (source
     (origin
       (method git-fetch)
       (uri (git-reference
             (url "https://github.com/ronaldoussoren/altgraph")
             (commit (string-append "v" version))))
       (file-name (git-file-name name version))
       (sha256
        (base32
         "09sm4srvvkw458pn48ga9q7ykr4xlz7q8gh1h9w7nxpf001qgpwb"))))
    (build-system python-build-system)
    (home-page "https://github.com/ronaldoussoren/altgraph")
    (synopsis "Python graph (network) package")
    (description "altgraph is a fork of graphlib: a graph (network) package for
constructing graphs, BFS and DFS traversals, topological sort, shortest paths,
etc. with graphviz output.")
    (license license:expat)))


(define-public python-macholib
  (package
    (name "python-macholib")
    (version "1.14")
    (source
     (origin
       (method git-fetch)
       (uri (git-reference
             (url "https://github.com/ronaldoussoren/macholib")
             (commit (string-append "v" version))))
       (file-name (git-file-name name version))
       (sha256
        (base32
         "0aislnnfsza9wl4f0vp45ivzlc0pzhp9d4r08700slrypn5flg42"))))
    (build-system python-build-system)
    (propagated-inputs
     `(("python-altgraph" ,python-altgraph)))
    (arguments
     '(#:phases
       (modify-phases %standard-phases
         (add-after 'unpack 'disable-broken-tests
           (lambda _
             ;; This test is broken as there is no keyboard interrupt.
             (substitute* "macholib_tests/test_command_line.py"
               (("^(.*)class TestCmdLine" line indent)
                (string-append indent
                               "@unittest.skip(\"Disabled by Guix\")\n"
                               line)))
             (substitute* "macholib_tests/test_dyld.py"
               (("^(.*)def test_\\S+_find" line indent)
                (string-append indent
                               "@unittest.skip(\"Disabled by Guix\")\n"
                               line))
               (("^(.*)def testBasic" line indent)
                (string-append indent
                               "@unittest.skip(\"Disabled by Guix\")\n"
                               line))
               )
             #t)))))
    (home-page "https://github.com/ronaldoussoren/macholib")
    (synopsis "Python library for analyzing and editing Mach-O headers")
    (description "macholib is a Macho-O header analyzer and editor. It's
typically used as a dependency analysis tool, and also to rewrite dylib
references in Mach-O headers to be @executable_path relative. Though this tool
targets a platform specific file format, it is pure python code that is platform
and endian independent.")
    (license license:expat)))

(define-public python-signapple
  (let ((commit "8a945a2e7583be2665cf3a6a89d665b70ecd1ab6"))
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
           "0fr1hangvfyiwflca6jg5g8zvg3jc9qr7vd2c12ff89pznf38dlg"))))
      (build-system python-build-system)
      (propagated-inputs
       `(("python-asn1crypto" ,python-asn1crypto)
         ("python-oscrypto" ,python-oscrypto)
         ("python-certvalidator" ,python-certvalidator)
         ("python-elfesteem" ,python-elfesteem)
         ("python-requests" ,python-requests)
         ("python-macholib" ,python-macholib)))
      ;; There are no tests, but attempting to run python setup.py test leads to
      ;; problems, just disable the test
      (arguments '(#:tests? #f))
      (home-page "https://github.com/achow101/signapple")
      (synopsis "Mach-O binary signature tool")
      (description "signapple is a Python tool for creating, verifying, and
inspecting signatures in Mach-O binaries.")
      (license license:expat))))

;; https://www.gnu.org/software/libc/manual/html_node/Configuring-and-compiling.html
;; We don't use --disable-werror directly, as that would be passed through to bash,
;; and cause it's build to fail.
(define (hardened-glibc glibc)
  (package-with-extra-configure-variable (
    package-with-extra-configure-variable (
      package-with-extra-configure-variable glibc
      "enable_werror" "no")
      "--enable-stack-protector" "all")
      "--enable-bind-now" "yes"))

(define-public glibc-2.27
  (package
    (inherit glibc-2.31)
    (version "2.27")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://sourceware.org/git/glibc.git")
                    (commit "73886db6218e613bd6d4edf529f11e008a6c2fa6")))
              (file-name (git-file-name "glibc" "73886db6218e613bd6d4edf529f11e008a6c2fa6"))
              (sha256
               (base32
                "0azpb9cvnbv25zg8019rqz48h8i2257ngyjg566dlnp74ivrs9vq"))
              (patches (search-our-patches "glibc-ldd-x86_64.patch"
                                           "glibc-versioned-locpath.patch"
                                           "glibc-2.27-riscv64-Use-__has_include-to-include-asm-syscalls.h.patch"
                                           "glibc-2.27-fcommon.patch"
                                           "glibc-2.27-guix-prefix.patch"))))))

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
        bzip2
        gzip
        xz
        ;; Build tools
        gnu-make
        libtool-2.4.7
        autoconf-2.71
        automake
        pkg-config
        bison
        ;; Native GCC 10 toolchain
        gcc-toolchain-10
        (list gcc-toolchain-10 "static")
        ;; Scripting
        python-minimal ;; (3.9)
        ;; Git
        git-minimal
        ;; Tests
        (fix-ppc64-nx-default python-lief))
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           ;; Windows
           (list zip
                 (make-mingw-pthreads-cross-toolchain "x86_64-w64-mingw32")
                 (make-nsis-for-gcc-10 nsis-x86_64)
                 nss-certs
                 osslsigncode))
          ((string-contains target "-linux-")
           (list (make-bitcoin-cross-toolchain target)))
          ((string-contains target "darwin")
           (list clang-toolchain-10 binutils cmake-minimal xorriso python-signapple))
          (else '())))))
