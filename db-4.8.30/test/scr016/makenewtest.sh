
# A script that helps create an empty test shell
# application.
# The only parameter is the name of the class.
# The output file will be in the src/com/sleepycat/db/test
# directory.


case $1 in
*Test)
;;
*)
  echo "New test case names must end in Test."
  exit 1;;
esac

outdir="src/com/sleepycat/db/test"
if [ ! -d $outdir ]
then
  echo "Could not find test source directory. Ensure the script is being run from the right place."
  exit 1
fi

outname="$outdir/$1.java"

if [ -f $outname ]
then
  echo "A test with that file name exists."
  echo -n "Are you sure you want to overwrite the file (yes/no)? "
  read got_ok
  if [ $got_ok != "yes" ]
  then
    exit 1
  else
    echo "" > $outname
  fi
fi

nameupper=`echo $1 | tr -t [:lower:] [:upper:]`
namelower=`echo $1 | tr -t [:upper:] [:lower:]`

echo "/*-" >> $outname
echo " * See the file LICENSE for redistribution information." >> $outname
echo " * " >> $outname
echo " * Copyright (c) 2002-2009 Oracle.  All rights reserved." >> $outname
echo " *" >> $outname
echo " */" >> $outname
echo "" >> $outname

echo "" >> $outname

echo "package com.sleepycat.db.test;" >> $outname
echo "" >> $outname
echo "import org.junit.After;" >> $outname
echo "import org.junit.AfterClass;" >> $outname
echo "import org.junit.Before;" >> $outname
echo "import org.junit.BeforeClass;" >> $outname
echo "import org.junit.Test;" >> $outname
echo "import static org.junit.Assert.assertEquals;" >> $outname
echo "import static org.junit.Assert.fail;" >> $outname
echo "" >> $outname
echo "import com.sleepycat.db.*;" >> $outname
echo "" >> $outname

# some other likely ones :)
echo "import java.io.File;" >> $outname
echo "import java.io.FileNotFoundException;" >> $outname
echo "import java.io.IOException;" >> $outname

echo "" >> $outname
echo "import com.sleepycat.db.test.TestUtils;" >> $outname

echo "public class $1 {" >> $outname

echo "    public static final String ${nameupper}_DBNAME = \"${namelower}.db\";" >> $outname

echo "    @BeforeClass public static void ClassInit() {" >> $outname
echo "        TestUtils.loadConfig(null);" >> $outname
echo -n "        TestUtils.check_file_removed(TestUtils." >> $outname
echo "getDBFileName(${nameupper}_DBNAME), true, true);" >> $outname
echo -n "        TestUtils.removeall(true, true, TestUtils." >> $outname
echo "BASETEST_DBDIR, TestUtils.getDBFileName(${nameupper}_DBNAME));" >> $outname
echo "    }" >> $outname

echo "" >> $outname
echo "    @AfterClass public static void ClassShutdown() {" >> $outname
echo -n "        TestUtils.check_file_removed(TestUtils." >> $outname
echo "getDBFileName(${nameupper}_DBNAME), true, true);" >> $outname
echo -n "        TestUtils.removeall(true, true, TestUtils." >> $outname
echo "BASETEST_DBDIR, TestUtils.getDBFileName(${nameupper}_DBNAME));" >> $outname
echo "    }" >> $outname

echo "" >> $outname
echo "    @Before public void PerTestInit()" >> $outname
echo "        throws Exception {" >> $outname
echo "    }" >> $outname

echo "" >> $outname
echo "    @After public void PerTestShutdown()" >> $outname
echo "        throws Exception {" >> $outname
echo "    }" >> $outname

echo "    /*" >> $outname
echo "     * Test case implementations." >> $outname
echo "     * To disable a test mark it with @Ignore" >> $outname
echo "     * To set a timeout(ms) notate like: @Test(timeout=1000)" >> $outname
echo "     * To indicate an expected exception notate like: $Test(expected=Exception)" >> $outname
echo "     */" >> $outname

echo "" >> $outname
echo "    @Test public void test1()" >> $outname
echo "        throws DatabaseException, FileNotFoundException" >> $outname
echo "    {" >> $outname
echo "    }" >> $outname

echo "}" >> $outname

