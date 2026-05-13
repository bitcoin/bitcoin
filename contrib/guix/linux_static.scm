(use-modules (gnu packages)
             (gnu packages base)
             (guix gexp)
             (guix packages)
             (toolchains))

(packages->manifest
 (append
  (let ((target (getenv "HOST")))
    (cond ((string-contains target "-linux-")
           (list (make-bitcoin-cross-toolchain target)))
          (else '())))))
