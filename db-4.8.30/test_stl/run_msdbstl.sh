#! /bin/sh

MS_TEST_PATH="ms_examples"

cnt=0
run_test()
{
prog=$1
rm -f ms_db_stl_result.out
$prog  -e d  -s h -c 67108864 < ./ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ./ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  -e d -s b -c 67108864 <./ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  -e c -s h -c 67108864 <ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  -e c -s b -c 67108864 <ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  -e t -s h -t a -c 67108864 <ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  -e t -s h -t e -c 67108864 <ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  -e t -s b -t a -c 67108864 <ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  -e t -s b -t e -c 67108864 <ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

rm -f ms_db_stl_result.out
$prog  <ms_examples/ms_db_stl.in > ms_db_stl_result.out
echo "ms db stl test and ms std stl test result diff: "
diff ms_db_stl_result.out ./ms_examples/ms_db_stl_result.expected > ms_db_stl-std_stl-results$cnt.diff
cnt=`expr $cnt + 1`

}
os=`uname -s`
#ms_example_std_stl
oldwd=`pwd`
if test $os = "CYGWIN_NT-5.1" ; then
	prog=../build_windows/Win32/Debug/stl_test_msexamples.exe
	run_test $prog 
	prog=../build_windows/Win32/Release/stl_test_msexamples.exe
	run_test $prog 
else
	prog=./ms_examples/ms_example_db_stl
	run_test $prog 
fi
cd $oldwd

