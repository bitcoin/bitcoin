<?php

// Open a new Db4Env
$dbenv = new Db4Env();
$dbenv->set_data_dir("/var/tmp/dbhome");
$dbenv->open("/var/tmp/dbhome");

// Open a database in $dbenv.  Note that even though
// we pass null in as the transaction, db4 forces this
// operation to be transactionally protected, so PHP
// will force auto-commit internally.
$db = new Db4($dbenv);
$db->open(null, 'a', 'foo');

$counter = $db->get("counter");
// Create a new transaction
$txn = $dbenv->txn_begin();
if($txn == false) {
  print "txn_begin failed";
  exit;
}
print "Current value of counter is $counter\n";

// Increment and reset counter, protect it with $txn
$db->put("counter", $counter+1, $txn);

// Commit the transaction, otherwise the above put() will rollback.
$txn->commit();
// Sync for good measure
$db->sync();
// This isn't a real close, use _close() for that.
$db->close();
?>
