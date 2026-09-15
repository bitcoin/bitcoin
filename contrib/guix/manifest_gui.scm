(use-modules (gnu packages bison)
             ((gnu packages build-tools) #:select (ninja))
             ((gnu packages compression) #:select (xz zip))
             (gnu packages gawk)
             ((gnu packages installers) #:select (nsis-x86_64))
             (gnu packages pkg-config)
             ((gnu packages python) #:select (python-minimal))
             ((gnu packages python-xyz) #:select (python-lief python-psutil))
             ((guix utils) #:select (substitute-keyword-arguments))
             (guix packages))

;; python-lief transitively pulls in python-psutil, which has
;; tests that fail when builing natively for riscv64.
;; See <https://codeberg.org/guix/guix/issues/10128>.
(define python-lief-no-psutil-tests
  ((package-mapping
    (lambda (p)
      (if (eq? p python-psutil)
          (package
            (inherit p)
            (arguments
             (substitute-keyword-arguments (package-arguments p)
               ((#:tests? _ #t) #f))))
          p)))
   python-lief))

(packages->manifest
 (append
  (list ;; Compression and archiving
        xz
        ;; Build tools
        ninja
        ;; Packaging scripts
        python-minimal ;; (3.12)
        ;; Tests
        python-lief-no-psutil-tests)
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
