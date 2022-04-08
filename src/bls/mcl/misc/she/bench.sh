for i in 4 6 8
do echo $i
touch test/she_test.cpp
make bin/she_test.exe CFLAGS_USER=-DMCLBN_FP_UNIT_SIZE=$i
bin/she_test.exe > misc/she/bench$i.txt
done
