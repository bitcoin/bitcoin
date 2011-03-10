<?php
# File should contain these lines:
#   mysql_connect('localhost', 'login', 'oass') or die(mysql_error());
#   mysql_select_db('database_name') or die(mysql_error());
require '/var/db.bitnomo.inc';

function fail($error)
{
    die('{"error": "'.$error.'"}');
}
function escapestr($str)
{
    return mysql_real_escape_string($str);
}
function do_query($query)
{
    $result = mysql_query($query) or fail(mysql_error());
    return $result;
}
function has_results($result)
{
    if (mysql_num_rows($result) > 0)
        return true;
    else
        return false;
}
?>

