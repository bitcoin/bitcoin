#run_test.sh
run_test()
{
prog=$1 
mkdir -p ./dbenv
rm -rf dbenv/*
echo "Auto commit btree transaction tests:"
$prog -s b -m t -t a  -T 200 -k 50 -l 100 -c 33554432 -n $2 >/dev/null
echo "Transaction btree tests:"
$prog -s b -m t -t e  -T 200 -k 50 -l 100 -c 33554432 -n $2 >/dev/null
echo "CDS btree tests:"
$prog -s b -m c -T 200 -k 50 -l 100 -c 33554432 -n $2  >/dev/null
echo "DS btree tests:"
$prog -s b -T 200 -k 50 -l 100 -c 33554432 -n $2  >/dev/null
echo "Auto commit hash transaction tests:"
$prog -s h -m t -t a  -T 200 -k 50 -l 100 -c 33554432 -n $2 >/dev/null
echo "Transaction hash tests:"
$prog -s h -m t -t e  -T 200 -k 50 -l 100 -c 33554432 -n $2 >/dev/null
echo "CDS hash tests:"
$prog -s h -m c -T 200 -k 50 -l 100 -c 33554432 -n $2 >/dev/null
echo "DS hash tests:"
$prog -s h -T 200 -k 50 -l 100 -c 33554432 -n $2  >/dev/null
}

run_mt()
{
prog=$1
echo "Transaction btree multithreaded tests:"
$prog -s b -m t -t e  -T 200 -k 50 -l 100 -c 33554432  -M > mt_test_btree_tds.out
echo "Transaction btree multithreaded tests(auto commit):"
$prog -s b -m t -t e  -T 200 -k 50 -l 100 -c 33554432  -M > mt_test_btree_tds_autocommit.out

echo "Transaction hash multithreaded tests:"
$prog -s h -m t -t e  -T 200 -k 50 -l 100 -c 33554432 -M > mt_test_hash_tds.out
echo "Transaction hash multithreaded tests:"
$prog -s h -m t -t e  -T 200 -k 50 -l 100 -c 33554432 -M > mt_test_hash_tds_autocommit.out
# This test is not suitable for CDS mode because the writers will not stop inserting to 
# the database until the reader reads enough key/data pairs, but the readers will never 
# have a chance to open its cursor read any key/data pair since the write is writing to db.
}
run_tpcb()
{
prog=$1

mkdir -p ./dbenv
rm -fr dbenv/*

$prog -i
$prog -n $2

}
os=`uname -s`
ntimes=1
mttest=0
if test "$1" = "tpcb" ; then #running tpcb test.
	if test $# -gt 1 ; then
		ntimes=$2
	fi

	if test "$os" = "CYGWIN_NT-5.1" ; then
		run_tpcb "../build_windows/Win32/Release/exstl_tpcb.exe" $ntimes
	else 
		run_tpcb  "../build_unix/exstl_tpcb" $ntimes
	fi
else
	if test "$1" = "mt" ; then #running ordinary test.
		mttest=1
	elif test $# -eq 2 ; then
		ntimes=$2
	fi
	
	if test "$os" = "CYGWIN_NT-5.1" ; then
		echo "Debug version test"
		if test $mttest -eq 1 ; then
			run_mt "../build_windows/Win32/Debug/stl_test.exe"
		else
			run_test "../build_windows/Win32/Debug/stl_test.exe" $ntimes
		fi
		echo "Release version test"
		if test $mttest -eq 1 ; then
			run_mt "../build_windows/Win32/Release/stl_test.exe"
		else
			run_test "../build_windows/Win32/Release/stl_test.exe" $ntimes
		fi
	else
		if test $mttest -eq 1 ; then
			run_mt "../build_unix/test_dbstl"
		else
			run_test "../build_unix/test_dbstl"  $ntimes
		fi
	fi
fi
