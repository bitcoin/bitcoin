TESTCLASSES=\
    ./src/com/sleepycat/bind/serial/test/*.java\
    ./src/com/sleepycat/bind/test/*.java\
    ./src/com/sleepycat/bind/tuple/test/*.java\
    ./src/com/sleepycat/collections/test/*.java\
    ./src/com/sleepycat/collections/test/serial/*.java\
    ./src/com/sleepycat/util/test/*.java \
    ./src/com/sleepycat/db/util/*.java

TESTSERIALPATH=com/sleepycat/collections/test/serial/TestSerial

all: dbtest.jar

dbtest.jar: classesdir
	# Compile the tests and build the test jar
	javac -classpath "${DB_JAR}${CP_SEP}${REQUIRED_JARS}" \
		-d ./classes ${TESTCLASSES}
	jar cf ./dbtest.jar -C ./classes ./com/sleepycat
	# Build the original version of TestSerial in the testserial directory
	mkdir -p "testserial/${TESTSERIALPATH}"
	cp "./src/${TESTSERIALPATH}.java.original" \
		"./testserial/${TESTSERIALPATH}.java"
	javac -classpath "${DB_JAR}${CP_SEP}${REQUIRED_JARS}" \
		-d ./testserial "testserial/${TESTSERIALPATH}.java"

classesdir:
	[ -d ./classes ] || (mkdir ./classes)

clean:
	[ -d ./classes ] && rm -rf ./classes
	[ -f ./dbtest.jar ] && rm ./dbtest.jar
	[ -d ./testserial ] && rm -rf ./testserial

