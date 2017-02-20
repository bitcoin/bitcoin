/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

package persist.gettingStarted;

import java.io.File;

import com.sleepycat.db.DatabaseException; 
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;

import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.StoreConfig;

import java.io.FileNotFoundException;

public class SimpleStoreGet {

    private static File envHome = new File("./JEDB");

    private Environment envmnt;
    private EntityStore store;
    private SimpleDA sda; 

   // The setup() method opens the environment and store
    // for us.
    public void setup()
        throws DatabaseException {

        try {
            EnvironmentConfig envConfig = new EnvironmentConfig();
            StoreConfig storeConfig = new StoreConfig();

            // Open the environment and entity store
            envmnt = new Environment(envHome, envConfig);
            store = new EntityStore(envmnt, "EntityStore", storeConfig);
        } catch (FileNotFoundException fnfe) {
            System.err.println("setup(): " + fnfe.toString());
            System.exit(-1);
        }
    } 

    public void shutdown()
        throws DatabaseException {

        store.close();
        envmnt.close();
    } 


    private void run()
        throws DatabaseException {

        setup();

        // Open the data accessor. This is used to store
        // persistent objects.
        sda = new SimpleDA(store);

        // Instantiate and store some entity classes
        SimpleEntityClass sec1 = sda.pIdx.get("keyone");
        SimpleEntityClass sec2 = sda.pIdx.get("keytwo");

        SimpleEntityClass sec4 = sda.sIdx.get("skeythree");

        System.out.println("sec1: " + sec1.getpKey());
        System.out.println("sec2: " + sec2.getpKey());
        System.out.println("sec4: " + sec4.getpKey());

        System.out.println("############ Doing pcursor ##########");
        for (SimpleEntityClass seci : sda.sec_pcursor ) {
                System.out.println("sec from pcursor : " + seci.getpKey() );
        }

        sda.pIdx.delete("keyone");
        System.out.println("############ Doing pcursor ##########");
        System.out.println("sec from pcursor : " + sda.sec_pcursor.first().getpKey());
        for (SimpleEntityClass seci : sda.sec_pcursor ) {
                System.out.println("sec from pcursor : " + seci.getpKey() );
        }

        System.out.println("############ Doing scursor ##########");
        for (SimpleEntityClass seci : sda.sec_scursor ) {
                System.out.println("sec from scursor : " + seci.getpKey() );
        }



        sda.close();
        shutdown();
    } 

    public static void main(String args[]) {
        SimpleStoreGet ssg = new SimpleStoreGet();
        try {
            ssg.run();
        } catch (DatabaseException dbe) {
            System.err.println("SimpleStoreGet: " + dbe.toString());
            dbe.printStackTrace();
        } catch (Exception e) {
            System.out.println("Exception: " + e.toString());
            e.printStackTrace();
        } 
        System.out.println("All done.");
    } 

}
