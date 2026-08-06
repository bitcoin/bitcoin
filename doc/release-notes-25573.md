Build
-----

Release binaries for x86_64 and aarch64 Linux (except
for bitcoin-qt) are now compiled with `-static-pie`,
making them more portable, i.e they can be run on musl-libc
based system, as well as on systems than ship with older glibcs,
than what we previously supported. (#25573)
