(use-modules (guix inferior) (guix channels)
             (srfi srfi-1))   ;for 'first'
(define channels
  (list (channel
         (name 'bitcoin-guix)
         (url "https://github.com/dongcarl/bitcoin-guix.git")
         (commit
          "9b221d425b000975b2e0d1632658f1558b9d2e05"))))

(define inferior
  ;; An inferior representing the above revision.
  (inferior-for-channels channels))

(packages->manifest
 (map (lambda (package-name)
        (first (lookup-inferior-packages inferior package-name)))
      '(;; The Basics
        "bash-minimal" ;; useful for debugging, can do with -minimal
        "coreutils"
        "which"
        "util-linux"
        ;; File(system) inspection
        "file"
        "grep"
        "diffutils"
        "findutils"
        ;; File transformation
        "patch"
        "gawk"
        "sed"
        ;; Compression and archiving
        "tar"
        "bzip2"
        "gzip"
        "xz"
        "zlib"
        ;; Build tools
        "make"
        "libtool"
        "autoconf"
        "automake"
        "pkg-config"
        ;; Scripting
        "perl"
        "python"
        ;; Native gcc 9 toolchain targeting glibc 2.27
        "gcc-glibc-2.27-toolchain"
        ;; Cross gcc 9 toolchains targeting glibc 2.27
        "i686-linux-gnu-toolchain"
        "x86_64-linux-gnu-toolchain"
        "aarch64-linux-gnu-toolchain"
        "arm-linux-gnueabihf-toolchain"
        "riscv64-linux-gnu-toolchain")))
