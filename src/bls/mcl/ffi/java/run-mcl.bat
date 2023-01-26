@echo off
echo [[compile MclTest.java]]
%JAVA_DIR%\bin\javac MclTest.java

echo [[run MclTest]]
set TOP_DIR=..\..
pushd %TOP_DIR%\bin
%JAVA_DIR%\bin\java -classpath ../ffi/java MclTest %1 %2 %3 %4 %5 %6
popd
