;; REDEFINE CARGO WITH THE RIGHT VERSION

(use-modules 
  (guix search-paths)
  (guix store)
  (guix utils)
  (guix derivations)
  (guix packages)
  (guix build-system)
  (guix build-system gnu)
  (ice-9 match)
  (ice-9 vlist)
  (srfi srfi-1)
  (srfi srfi-26))

(define %crate-base-url
  (make-parameter "https://crates.io"))
(define crate-url
  (string-append (%crate-base-url) "/api/v1/crates/"))
(define crate-url?
  (cut string-prefix? crate-url <>))

(define (crate-uri name version)
  "Return a URI string for the crate package hosted at crates.io corresponding
to NAME and VERSION."
  (string-append crate-url name "/" version "/download"))

(define (default-rust)
  "Return the default Rust package."
  ;; Lazily resolve the binding to avoid a circular dependency.
  (let ((rust (resolve-interface '(gnu packages rust))))
    (module-ref rust 'rust-1.51)))


(define %cargo-utils-modules
  ;; Build-side modules imported by default.
  `((guix build cargo-utils)
    ,@%gnu-build-system-modules))

(define %cargo-build-system-modules
  ;; Build-side modules imported by default.
  `((guix build cargo-build-system)
    (guix build json)
    ,@%cargo-utils-modules))

(define* (cargo-build store name inputs
                      #:key
                      (tests? #t)
                      (test-target #f)
                      (vendor-dir "guix-vendor")
                      (cargo-build-flags ''("--release"))
                      (cargo-test-flags ''("--release"))
                      (cargo-package-flags ''("--no-metadata" "--no-verify"))
                      (features ''())
                      (skip-build? #f)
                      (install-source? #t)
                      (phases '(@ (guix build cargo-build-system)
                                  %standard-phases))
                      (outputs '("out"))
                      (search-paths '())
                      (system (%current-system))
                      (guile #f)
                      (imported-modules %cargo-build-system-modules)
                      (modules '((guix build cargo-build-system)
                                 (guix build utils))))
  "Build SOURCE using CARGO, and with INPUTS."

  (define builder
    `(begin
       (use-modules ,@modules)
       (cargo-build #:name ,name
                    #:source ,(match (assoc-ref inputs "source")
                                (((? derivation? source))
                                 (derivation->output-path source))
                                ((source)
                                 source)
                                (source
                                 source))
                    #:system ,system
                    #:test-target ,test-target
                    #:vendor-dir ,vendor-dir
                    #:cargo-build-flags ,cargo-build-flags
                    #:cargo-test-flags ,cargo-test-flags
                    #:cargo-package-flags ,cargo-package-flags
                    #:features ,features
                    #:skip-build? ,skip-build?
                    #:install-source? ,install-source?
                    #:tests? ,(and tests? (not skip-build?))
                    #:phases ,phases
                    #:outputs %outputs
                    #:search-paths ',(map search-path-specification->sexp
                                          search-paths)
                    #:inputs %build-inputs)))

  (define guile-for-build
    (match guile
      ((? package?)
       (package-derivation store guile system #:graft? #f))
      (#f                                         ; the default
       (let* ((distro (resolve-interface '(gnu packages commencement)))
              (guile  (module-ref distro 'guile-final)))
         (package-derivation store guile system #:graft? #f)))))

  (build-expression->derivation store name builder
                                #:inputs inputs
                                #:system system
                                #:modules imported-modules
                                #:outputs outputs
                                #:guile-for-build guile-for-build))

(define (package-cargo-inputs p)
  (apply
    (lambda* (#:key (cargo-inputs '()) #:allow-other-keys)
      cargo-inputs)
    (package-arguments p)))

(define (package-cargo-development-inputs p)
  (apply
    (lambda* (#:key (cargo-development-inputs '()) #:allow-other-keys)
      cargo-development-inputs)
    (package-arguments p)))

(define (crate-closure inputs)
  "Return the closure of INPUTS when considering the 'cargo-inputs' and
'cargod-dev-deps' edges.  Omit duplicate inputs, except for those
already present in INPUTS itself.

This is implemented as a breadth-first traversal such that INPUTS is
preserved, and only duplicate extracted inputs are removed.

Forked from ((guix packages) transitive-inputs) since this extraction
uses slightly different rules compared to the rest of Guix (i.e. we
do not extract the conventional inputs)."
  (define (seen? seen item)
    ;; FIXME: We're using pointer identity here, which is extremely sensitive
    ;; to memoization in package-producing procedures; see
    ;; <https://bugs.gnu.org/30155>.
    (vhash-assq item seen))

  (let loop ((inputs     inputs)
             (result     '())
             (propagated '())
             (first?     #t)
             (seen       vlist-null))
    (match inputs
      (()
       (if (null? propagated)
           (reverse result)
           (loop (reverse (concatenate propagated)) result '() #f seen)))
      (((and input (label (? package? package))) rest ...)
       (if (and (not first?) (seen? seen package))
           (loop rest result propagated first? seen)
           (loop rest
                 (cons input result)
                 (cons (package-cargo-inputs package)
                       propagated)
                 first?
                 (vhash-consq package package seen))))
      ((input rest ...)
       (loop rest (cons input result) propagated first? seen)))))

(define (expand-crate-sources cargo-inputs cargo-development-inputs)
  "Extract all transitive sources for CARGO-INPUTS and CARGO-DEVELOPMENT-INPUTS
along their 'cargo-inputs' edges.

Cargo requires all transitive crate dependencies' sources to be available
in its index, even if they are optional (this is so it can generate
deterministic Cargo.lock files regardless of the target platform or enabled
features). Thus we need all transitive crate dependencies for any cargo
dev-dependencies, but this is only needed when building/testing a crate directly
(i.e. we will never need transitive dev-dependencies for any dependency crates).

Another complication arises due potential dependency cycles from Guix's
perspective: Although cargo does not permit cyclic dependencies between crates,
however, it permits cycles to occur via dev-dependencies. For example, if crate
X depends on crate Y, crate Y's tests could pull in crate X to to verify
everything builds properly (this is a rare scenario, but it it happens for
example with the `proc-macro2` and `quote` crates). This is allowed by cargo
because tests are built as a pseudo-crate which happens to depend on the
X and Y crates, forming an acyclic graph.

We can side step this problem by only considering regular cargo dependencies
since they are guaranteed to not have cycles. We can further resolve any
potential dev-dependency cycles by extracting package sources (which never have
any dependencies and thus no cycles can exist).

There are several implications of this decision:
* Building a package definition does not require actually building/checking
any dependent crates. This can be a benefits:
 - For example, sometimes a crate may have an optional dependency on some OS
 specific package which cannot be built or run on the current system. This
 approach means that the build will not fail if cargo ends up internally ignoring
 the dependency.
 - It avoids waiting for quadratic builds from source: cargo always builds
 dependencies within the current workspace. This is largely due to Rust not
 having a stable ABI and other resolutions that cargo applies. This means that
 if we have a depencency chain of X -> Y -> Z and we build each definition
 independently the following will happen:
  * Cargo will build and test crate Z
  * Cargo will build crate Z in Y's workspace, then build and test Y
  * Cargo will build crates Y and Z in X's workspace, then build and test X
* But there are also some downsides with this approach:
  - If a dependent crate is subtly broken on the system (i.e. it builds but its
  tests fail) the consuming crates may build and test successfully but
  actually fail during normal usage (however, the CI will still build all
  packages which will give visibility in case packages suddenly break).
  - Because crates aren't declared as regular inputs, other Guix facilities
  such as tracking package graphs may not work by default (however, this is
  something that can always be extended or reworked in the future)."
  (filter-map
    (match-lambda
      ((label (? package? p))
       (list label (package-source p)))
      ((label input)
       (list label input)))
    (crate-closure (append cargo-inputs cargo-development-inputs))))

(define* (lower name
                #:key source inputs native-inputs outputs system target
                (rust (default-rust))
                (cargo-inputs '())
                (cargo-development-inputs '())
                #:allow-other-keys
                #:rest arguments)
  "Return a bag for NAME."

  (define private-keywords
    '(#:source #:target #:rust #:inputs #:native-inputs #:outputs
      #:cargo-inputs #:cargo-development-inputs))

  (and (not target) ;; TODO: support cross-compilation
       (bag
         (name name)
         (system system)
         (target target)
         (host-inputs `(,@(if source
                              `(("source" ,source))
                              '())
                        ,@inputs

                        ;; Keep the standard inputs of 'gnu-build-system'
                        ,@(standard-packages)))
         (build-inputs `(("cargo" ,rust "cargo")
                         ("rustc" ,rust)
                         ,@(expand-crate-sources cargo-inputs cargo-development-inputs)
                         ,@native-inputs))
         (outputs outputs)
         (build cargo-build)
         (arguments (strip-keyword-arguments private-keywords arguments)))))

(define cargo-build-system
  (build-system
    (name 'cargo)
    (description
     "Cargo build system, to build Rust crates")
    (lower lower)))

;; COMPILE THE RUST PACKEGES

(use-modules (gnu)
  (guix licenses)
  ;(guix build-system cargo2)
  (guix build-system copy)
  (guix download)
  (guix packages)
  (gnu packages bash)
  (gnu packages crates-io)
  (gnu packages rust-apps)
  (gnu packages crates-graphics)
  (gnu packages documentation)
  (gnu packages fontutils)
  (gnu packages version-control)
  (gnu packages file)
  (gnu packages linux)
  (gnu packages gawk)
  (gnu packages compression)
  (gnu packages autotools)
  (gnu packages gcc)
  (gnu packages pkg-config))



(define-public rust-binary_codec_sv2-0.1.1
    (package
      (name "rust-binary_codec_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/binary_codec_sv2-0.1.1.crate")
         (sha256
          (base32 "1d9nacfmb6ljp49690q63227l8k87j1h3y9hhb99bw5ispz8idbx"))))
      (build-system cargo-build-system)

      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )

(define-public rust-derive_codec_sv2-0.1.1
    (package
      (name "rust-derive_codec_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/derive_codec_sv2-0.1.1.crate")
         (sha256
          (base32 "0vjy4dv7nvydf6i2nnaiwfvnsfrmkw1vbnmxc75rkp41syp76yqx"))))
      (build-system cargo-build-system)
        (arguments
         `(
           #:cargo-inputs
           (("binary_codec_sv2" , rust-binary_codec_sv2-0.1.1)
            )))

      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )
(define-public rust-binary_sv2-0.1.3
    (package
      (name "rust-binary_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/binary_sv2-0.1.3.crate")
         (sha256
          (base32 "0nx6y8lsgmq14z9r43vw2paprn3xk1aq81zw1wwkgw9y13q426if"))))
      (build-system cargo-build-system)
        (arguments
         `(
           #:skip-build? #t
           #:cargo-inputs
           (
            ("binary_codec_sv2" , rust-binary_codec_sv2-0.1.1)
            ("derive_codec_sv2" , rust-derive_codec_sv2-0.1.1)
            ("rust-derive_codec_sv2", rust-derive_codec_sv2-0.1.1 )
            )))

      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )
(define-public rust-const_sv2-0.1.0
    (package
      (name "rust-const_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/const_sv2-0.1.0.crate")
         (sha256
          (base32 "1qgr8fpjza8dk7r60nrnaicyazz75w9a99yq34rsv6x8finyrbdi"))))
      (build-system cargo-build-system)
      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )
(define-public rust-framing_sv2-0.1.3
    (package
      (name "rust-framing_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/framing_sv2-0.1.3.crate")
         (sha256
          (base32 "06z8lw10a85cgbx8lw0v6wggddr7w0463vcc05h11rc57cjpbir6"))))
      (build-system cargo-build-system)
        (arguments
         `(
           #:skip-build? #t
           #:cargo-inputs
           (
            ("binary_sv2" , rust-binary_sv2-0.1.3)
            ("const_sv2" , rust-const_sv2-0.1.0)
            )))

      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )
(define-public rust-codec_sv2-0.1.3
    (package
      (name "rust-codec_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/codec_sv2-0.1.3.crate")
         (sha256
          (base32 "0c10bnpkh96k1m92yjmzxa3qhkpc2wc6kmpgv44v1zfkk016nxyp"))))
      (build-system cargo-build-system)
        (arguments
         `(
           #:skip-build? #t
           #:cargo-inputs
           (
            ("binary_sv2" , rust-binary_sv2-0.1.3)
            ("const_sv2" , rust-const_sv2-0.1.0)
            ("framing-sv2" , rust-framing_sv2-0.1.3)
            )))

      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )
(define-public rust-common_messages_sv2-0.1.3
    (package
      (name "rust-common_messages_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/common_messages_sv2-0.1.3.crate")
         (sha256
          (base32 "1wsyy44h3pirrb08brz9gr9g4mshxcpf37i557dxyz79c0gkl3lj"))))
      (build-system cargo-build-system)
        (arguments
         `(
           #:skip-build? #t
           #:cargo-inputs
           (
            ("binary_sv2" , rust-binary_sv2-0.1.3)
            ("const_sv2" , rust-const_sv2-0.1.0)
            )))

      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )
(define-public rust-template_distribution_sv2-0.1.3
    (package
      (name "rust-template_distribution_sv2")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/template_distribution_sv2-0.1.3.crate")
         (sha256
          (base32 "05dm5si3h09pz05vk7zlg0w0vrkiq5yh9gr043i472rny101x5ki"))))
      (build-system cargo-build-system)
        (arguments
         `(
           #:skip-build? #t
           #:cargo-inputs
           (
            ("binary_sv2" , rust-binary_sv2-0.1.3)
            ("const_sv2" , rust-const_sv2-0.1.0)
            )))

      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )

(define-public rust-sv2_ffi-0.1.3
    (package
      (name "rust-sv2_ffi")
      (version "0.1.0")
      (source
       (origin
         (method url-fetch)
         (uri "./src/rusty/target/package/sv2_ffi-0.1.3.crate")
         (sha256
          (base32 "01a6lj49hv9flxjqnjk01zza0664gj4mm4hh74wi2jch2mfi1yh5"))))
      (build-system cargo-build-system)
      (arguments
       `(
         #:cargo-inputs
         (
          ("binary_sv2" , rust-binary_sv2-0.1.3)
          ("codec_sv2" , rust-codec_sv2-0.1.3)
          ("const_sv2" , rust-const_sv2-0.1.0)
          ("common-messages_sv2" , rust-common_messages_sv2-0.1.3)
          ("template-distribution_sv2" , rust-template_distribution_sv2-0.1.3)
          )
         #:phases
         (modify-phases %standard-phases
              (replace 'install
                (lambda* (#:key inputs outputs skip-build? features install-source? #:allow-other-keys)
                  (let* ((out (assoc-ref outputs "out")))
                    (mkdir-p out)
                
                    (install-file "./sv2.h" out)
                    (install-file "./target/release/libsv2_ffi.a" out)
                  #true)))
              )
       ))
      (home-page "..")
      (synopsis "..")
      (description
       "..")
      (license gpl3+))
    )

;;(packages->manifest
;; (append
;;  (list ;; The Basics
;;        bash
;;        which
;;        coreutils
;;        util-linux
;;        ;;; File(system) inspection
;;        file
;;        grep
;;        diffutils
;;        findutils
;;        ;;; File transformation
;;        patch
;;        gawk
;;        sed
;;        ;;; Compression and archiving
;;        tar
;;        bzip2
;;        gzip
;;        xz
;;        zlib
;;        ;;; Build tools
;;        gnu-make
;;        libtool
;;        autoconf
;;        automake
;;        pkg-config
;;        ;rust-sv2_example
;;        rust-binary_codec_sv2-0.1.1
;;        rust-derive_codec_sv2-0.1.1
;;        rust-binary_sv2-0.1.3
;;        rust-const_sv2-0.1.0
;;        rust-framing_sv2-0.1.3
;;        rust-codec_sv2-0.1.3
;;        rust-common_messages_sv2-0.1.3
;;        rust-template_distribution_sv2-0.1.3
;;        rust-sv2_ffi-0.1.3
;;        gcc
;;  )))

;; ######################################3
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
             (gnu packages perl)
             (gnu packages pkg-config)
             (gnu packages python)
             (gnu packages python-crypto)
             (gnu packages python-web)
             (gnu packages shells)
             (gnu packages tls)
             (gnu packages version-control)
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

(define base-gcc gcc-10)
(define base-linux-kernel-headers linux-libre-headers-5.15)

;; Building glibc with stack smashing protector first landed in glibc 2.25, use
;; this function to disable for older glibcs
;;
;; From glibc 2.25 changelog:
;;
;;   * Most of glibc can now be built with the stack smashing protector enabled.
;;     It is recommended to build glibc with --enable-stack-protector=strong.
;;     Implemented by Nick Alcock (Oracle).
(define (make-glibc-without-ssp xglibc)
  (package-with-extra-configure-variable
   (package-with-extra-configure-variable
    xglibc "libc_cv_ssp" "no")
   "libc_cv_ssp_strong" "no"))

(define* (make-bitcoin-cross-toolchain target
                                       #:key
                                       (base-gcc-for-libc gcc-7)
                                       (base-kernel-headers base-linux-kernel-headers)
                                       (base-libc (make-glibc-without-ssp glibc-2.24))
                                       (base-gcc (make-gcc-rpath-link base-gcc)))
  "Convenience wrapper around MAKE-CROSS-TOOLCHAIN with default values
desirable for building Bitcoin Core release binaries."
  (make-cross-toolchain target
                        base-gcc-for-libc
                        base-kernel-headers
                        base-libc
                        base-gcc))

(define (make-gcc-with-pthreads gcc)
  (package-with-extra-configure-variable gcc "--enable-threads" "posix"))

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
    (search-our-patches "nsis-gcc-10-memmove.patch")))

(define-public lief
  (package
   (name "python-lief")
   (version "0.12.0")
   (source
    (origin
     (method git-fetch)
     (uri (git-reference
           (url "https://github.com/lief-project/LIEF.git")
           (commit version)))
     (file-name (git-file-name name version))
     (sha256
      (base32
       "026jchj56q25v6gc0754dj9cj5hz5zaza8ij93y5ga94w20kzm9q"))))
   (build-system python-build-system)
   (native-inputs
    `(("cmake" ,cmake)))
   (home-page "https://github.com/lief-project/LIEF")
   (synopsis "Library to Instrument Executable Formats")
   (description "Python library to to provide a cross platform library which can
parse, modify and abstract ELF, PE and MachO formats.")
   (license license:asl2.0)))

(define osslsigncode
  (package
    (name "osslsigncode")
    (version "2.0")
    (source (origin
              (method url-fetch)
              (uri (string-append "https://github.com/mtrojnar/"
                                  name "/archive/" version ".tar.gz"))
              (sha256
               (base32
                "0byri6xny770wwb2nciq44j5071122l14bvv65axdd70nfjf0q2s"))))
    (build-system gnu-build-system)
    (native-inputs
     `(("pkg-config" ,pkg-config)
       ("autoconf" ,autoconf)
       ("automake" ,automake)
       ("libtool" ,libtool)))
    (inputs
     `(("openssl" ,openssl)))
    (arguments
     `(#:configure-flags
       `("--without-gsf"
         "--without-curl"
         "--disable-dependency-tracking")))
    (home-page "https://github.com/mtrojnar/osslsigncode")
    (synopsis "Authenticode signing and timestamping tool")
    (description "osslsigncode is a small tool that implements part of the
functionality of the Microsoft tool signtool.exe - more exactly the Authenticode
signing and timestamping. But osslsigncode is based on OpenSSL and cURL, and
thus should be able to compile on most platforms where these exist.")
    (license license:gpl3+))) ; license is with openssl exception

(define-public python-elfesteem
  (let ((commit "87bbd79ab7e361004c98cc8601d4e5f029fd8bd5"))
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
           "1nyvjisvyxyxnd0023xjf5846xd03lwawp5pfzr8vrky7wwm5maz"))
      (patches (search-our-patches "elfsteem-value-error-python-39.patch"))))
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

(define-public glibc-2.24
  (package
    (inherit glibc-2.31)
    (version "2.24")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://sourceware.org/git/glibc.git")
                    (commit "0d7f1ed30969886c8dde62fbf7d2c79967d4bace")))
              (file-name (git-file-name "glibc" "0d7f1ed30969886c8dde62fbf7d2c79967d4bace"))
              (sha256
               (base32
                "0g5hryia5v1k0qx97qffgwzrz4lr4jw3s5kj04yllhswsxyjbic3"))
              (patches (search-our-patches "glibc-ldd-x86_64.patch"
                                           "glibc-versioned-locpath.patch"
                                           "glibc-2.24-elfm-loadaddr-dynamic-rewrite.patch"
                                           "glibc-2.24-no-build-time-cxx-header-run.patch"))))))

(define-public glibc-2.27/bitcoin-patched
  (package
    (inherit glibc-2.31)
    (version "2.27")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://sourceware.org/git/glibc.git")
                    (commit "23158b08a0908f381459f273a984c6fd328363cb")))
              (file-name (git-file-name "glibc" "23158b08a0908f381459f273a984c6fd328363cb"))
              (sha256
               (base32
                "1b2n1gxv9f4fd5yy68qjbnarhf8mf4vmlxk10i3328c1w5pmp0ca"))
              (patches (search-our-patches "glibc-ldd-x86_64.patch"
                                           "glibc-2.27-riscv64-Use-__has_include__-to-include-asm-syscalls.h.patch"))))))

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
        moreutils
        ;; Compression and archiving
        tar
        bzip2
        gzip
        xz
        ;; Build tools
        gnu-make
        libtool
        autoconf-2.71
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
        ;; RUST
        rust-binary_codec_sv2-0.1.1
        rust-derive_codec_sv2-0.1.1
        rust-binary_sv2-0.1.3
        rust-const_sv2-0.1.0
        rust-framing_sv2-0.1.3
        rust-codec_sv2-0.1.3
        rust-common_messages_sv2-0.1.3
        rust-template_distribution_sv2-0.1.3
        rust-sv2_ffi-0.1.3
        (list gcc-toolchain-7 "static"))
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           ;; Windows
           (list ;; Native GCC 10 toolchain
                 gcc-toolchain-10
                 (list gcc-toolchain-10 "static")
                 zip
                 (make-mingw-pthreads-cross-toolchain "x86_64-w64-mingw32")
                 (make-nsis-for-gcc-10 nsis-x86_64)
                 osslsigncode))
          ((string-contains target "-linux-")
           (list ;; Native GCC 7 toolchain
                 gcc-toolchain-7
                 (list gcc-toolchain-7 "static")
                 (cond ((string-contains target "riscv64-")
                        (make-bitcoin-cross-toolchain target
                                                      #:base-libc glibc-2.27/bitcoin-patched
                                                      #:base-kernel-headers base-linux-kernel-headers))
                       (else
                        (make-bitcoin-cross-toolchain target)))))
          ((string-contains target "darwin")
           (list ;; Native GCC 10 toolchain
                 gcc-toolchain-10
                 (list gcc-toolchain-10 "static")
                 clang-toolchain-10 binutils cmake xorriso python-signapple))
          (else '())))))
