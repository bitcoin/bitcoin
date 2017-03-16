#!/bin/bash
cat > run.gdbscript << FIN
set \$_exitcode = -999
set height 0
handle SIGTERM nostop print pass
handle SIGPIPE nostop
define hook-stop
    if \$_exitcode != -999
        quit \$_exitcode
    else
        thread apply all backtrace
        quit 1
    end
end
echo .gdbinit: running app\n
run
FIN

gdb -x run.gdbscript -args src/test/test_bitcoin --catch_system_errors=no
