#! /bin/sh

JAVA=java
JAVAC=javac

$JAVAC -cp miniupnpc_Linux.jar JavaBridgeTest.java
$JAVA -cp miniupnpc_Linux.jar:. JavaBridgeTest 12345 UDP

