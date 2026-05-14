(use-modules (toolchains))

(packages->manifest
 (append
  (let ((target (getenv "HOST")))
    (cond ((string-contains target "x86_64-linux-")
           (list (make-bitcoin-cross-toolchain target
                                               #:base-libc glibc-2.44)))
          ((string-contains target "-linux-")
           (list (make-bitcoin-cross-toolchain target)))
          (else '())))))
