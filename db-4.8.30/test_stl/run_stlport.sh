run_test()
{
prog=$1
out=$2.out
dif=$2.diff
rm -f $out
rm -f $dif
$prog > $out
if [ $# -gt 2 ]; then
	dos2unix -U $out 
fi
diff $out  ./stlport/stl_test.expected > $dif
}

os=`uname -s`
if test $os = "CYGWIN_NT-5.1" ; then
	prog=../build_windows/Win32/Debug/stl_test_stlport.exe
	run_test $prog  "stlport_results_dbg" 1
	prog=../build_windows/Win32/Release/stl_test_stlport.exe
	run_test $prog "stlport_results_rel" 1
else
	prog=./stlport/stl_port_test
	run_test $prog "stlport_results"
fi
