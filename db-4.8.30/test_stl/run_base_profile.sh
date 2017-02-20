#run_test.sh
run_test()
{
prog=$1 
mkdir -p ./dbenv
rm -f -r dbenv/*

echo "Auto commit btree transaction tests:" >&2
	$prog -s b -m t -t a  -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	

echo "Transaction btree tests:" >&2
	$prog -s b -m t -t e  -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	
echo "CDS btree tests:" >&2
	$prog -s b -m c -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	

echo "DS btree tests:" >&2
	$prog -s b -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	

echo "Auto commit hash transaction tests:" >&2
	$prog -s h -m t -t a  -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	

echo "Transaction hash tests:" >&2
	$prog -s h -m t -t e  -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	

echo "CDS hash tests:" >&2
	$prog -s h -m c -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	

echo "DS hash tests:" >&2
	$prog -s h -T 200 -k 50 -l 100 -c 33554432 -n $total && mv gmon.out gmon${i}.out
	i=`expr $i + 1`	
}

if test $# -ne 1 ; then
	echo "Usage: sh run_test.sh number-of-run-in-a-loop"
	exit 1
fi

total=$1
i=0
os=`uname -s`

if test $os = "CYGWIN_NT-5.1" ; then
	run_test "../build_windows/Win32/Debug/test.exe"
else
	run_test "../build_unix/test_dbstl"
fi

ii=0
profiles=""

while [ $ii -lt $i ]; do
	profiles="${profiles} gmon${ii}.out"
	ii=`expr $ii + 1`	
done
echo "Generating profiling report..." >&2
gprof $1/.libs/test_dbstl $profiles > gprof.out
