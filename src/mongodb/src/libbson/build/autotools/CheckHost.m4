AC_CANONICAL_HOST

os_win32=no
os_linux=no
os_freebsd=no
os_gnu=no

case "$host" in
    *-mingw*|*-*-cygwin*)
        os_win32=yes
        TARGET_OS=windows
        ;;
    *-*-*netbsd*)
        os_netbsd=yes
	TARGET_OS=unix
        ;;
    *-*-*freebsd*)
        os_freebsd=yes
	TARGET_OS=unix
        ;;
    *-*-*openbsd*)
        os_openbsd=yes
	TARGET_OS=unix
        ;;
    *-*-linux*)
        os_linux=yes
        os_gnu=yes
	TARGET_OS=unix
        ;;
    *-*-solaris*)
        os_solaris=yes
	TARGET_OS=unix
        ;;
    *-*-darwin*)
        os_darwin=yes
	TARGET_OS=unix
        ;;
    gnu*|k*bsd*-gnu*)
        os_gnu=yes
	TARGET_OS=unix
        ;;
    *)
        AC_MSG_WARN([*** Please add $host to configure.ac checks!])
        ;;
esac
