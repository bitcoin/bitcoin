(use-modules (toolchains))

(packages->manifest
 (append
  (let ((target (getenv "HOST")))
    (cond ((string-contains target "-linux-")
           (list (make-bitcoin-cross-toolchain target
                                               #:base-gcc-for-libc linux-base-gcc-minimal
                                               #:base-gcc linux-base-gcc-minimal)))
          (else '())))))
