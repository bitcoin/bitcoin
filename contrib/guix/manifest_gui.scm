(use-modules (gnu packages bison)
             ((gnu packages check) #:select (python-pytest-xprocess))
             ((gnu packages compression) #:select (xz zip))
             (gnu packages gawk)
             ((gnu packages installers) #:select (nsis-x86_64))
             (gnu packages ninja)
             (gnu packages pkg-config)
             ((gnu packages python) #:select (python-minimal))
             ((gnu packages python-xyz) #:select (python-lief python-psutil python-sh))
             ((guix utils) #:select (substitute-keyword-arguments))
             (guix packages))

;; python-lief transitively pulls in python-psutil and
;; python-pytest-xprocess, which have tests that fail
;; when building natively on riscv64.
;; See <https://codeberg.org/guix/guix/issues/10128>.
(define (package-without-tests p)
  (package
    (inherit p)
    (arguments
     (substitute-keyword-arguments (package-arguments p)
       ((#:tests? _ #t) #f)))))

(define python-lief-no-riscv64-failing-tests
  ((package-mapping
    (lambda (p)
      (if (memq p (list python-psutil python-pytest-xprocess python-sh))
          (package-without-tests p)
          p)))
   python-lief))

(packages->manifest
 (append
  (list ;; Compression and archiving
        xz
        ;; Build tools
        ninja
        ;; Packaging scripts
        python-minimal ;; 3.12
        ;; Tests
        python-lief-no-riscv64-failing-tests) ;; 0.17.6
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           (list zip
                 nsis-x86_64))
          ((string-contains target "-linux-")
           (list bison
                 gawk
                 pkg-config))
          ((string-contains target "darwin")
           (list zip))
          (else '())))))
