TESTCLASSES=\
    ./src/com/sleepycat/db/test/*.java

all: dbtest.jar

dbtest.jar: classesdir
	# Compile the tests and build the test jar
	javac -classpath "${DB_JAR}${CP_SEP}${REQUIRED_JARS}" \
		-d ./classes ${TESTCLASSES}
	jar cf ./dbtest.jar -C ./classes ./com/sleepycat

classesdir:
	[ -d ./classes ] || (mkdir ./classes)

clean:
	[ -d ./classes ] && rm -rf ./classes
	[ -f ./dbtest.jar ] && rm ./dbtest.jar

