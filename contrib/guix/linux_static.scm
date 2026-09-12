(use-modules (toolchains))

(packages->manifest
 (append
  (let ((target (getenv "HOST")))
    (cond ((string-contains target "-linux-")
           (list (make-bitcoin-cross-toolchain target)))
          (else '())))))
