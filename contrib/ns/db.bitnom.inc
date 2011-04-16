<?php
// This will connect to the MySQL server
mysql_connect('localhost', 'root', '') or die(mysql_error());
// This will connect to a database on that server.
mysql_select_db('bitnom') or die(mysql_error());

