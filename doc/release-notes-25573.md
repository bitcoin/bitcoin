Build
-----

Release binaries for x86_64 and aarch64 Linux (except
for bitcoin-qt) are now compiled with `-static-pie`,
making them more portable. They can now be run on non-glibc
based systems, as well as on systems than ship with older
glibcs, than were previously supported.

A side-effect of this change is that name resolution through
dynamic nss modules is not possible. Users using mdns (.local),
LDAP, or SSSD to resolve names for local connections should
verify that connections are still resolved correctly. (#25573)
