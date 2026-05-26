(use-modules ((gnu packages bash) #:select (bash-minimal))
             ((gnu packages compression) #:select (gzip zip))
             ((gnu packages crypto) #:select (osslsigncode))
             ((gnu packages python-build) #:select (python-poetry-core))
             ((gnu packages python-crypto) #:select (python-asn1crypto python-oscrypto))
             ((gnu packages tls) #:select (openssl))
             ((gnu packages version-control) #:select (git-minimal))
             (guix build-system python)
             (guix build-system pyproject)
             (guix git-download)
             ((guix licenses) #:prefix license:)
             (guix packages))

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
        (list openssl
              python-asn1crypto
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
  (let ((commit "3fab3bb57f227f0dd31007b417683035f5204838"))
    (package
      (name "python-signapple")
      (version (git-version "0.2.0" "1" commit))
      (source
       (origin
         (method git-fetch)
         (uri (git-reference
               (url "https://github.com/achow101/signapple")
               (commit commit)))
         (file-name (git-file-name name commit))
         (sha256
          (base32
           "0qpr78bs50rw79dbihr9ifjq19y6819ih5pn9jd2rbjyifimzf7p"))))
      (build-system pyproject-build-system)
      (propagated-inputs
        (list python-asn1crypto
              python-oscrypto
              python-certvalidator
              python-elfesteem))
      (native-inputs (list python-poetry-core))
      (arguments
       ;; There are no tests, but attempting to run python setup.py test leads to
       ;; problems, just disable the test
       (list #:tests? #f
             #:phases
             #~(modify-phases %standard-phases
                 ;; Add a phase to inject OpenSSL paths for oscrypto.
                 (add-after 'wrap 'wrap-openssl-paths
                   (lambda* (#:key inputs #:allow-other-keys)
                     (let ((openssl (assoc-ref inputs "openssl")))
                       (wrap-program (string-append #$output "/bin/signapple")
                         `("SIGNAPPLE_OSCRYPTO_SSL_PATHS" =
                           (,(string-append openssl "/lib/libcrypto.so" "," openssl "/lib/libssl.so"))))))))))
      (home-page "https://github.com/achow101/signapple")
      (synopsis "Mach-O binary signature tool")
      (description "signapple is a Python tool for creating, verifying, and
inspecting signatures in Mach-O binaries.")
      (license license:expat))))

(packages->manifest
 (append
  (list ;; The Basics
        bash-minimal
        coreutils-minimal
        ;; File(system) inspection
        findutils
        ;; Compression and archiving
        tar
        gzip
        zip
        ;; Git
        git-minimal)
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           (list osslsigncode))
          ((string-contains target "darwin")
           (list python-signapple))
          (else '())))))
