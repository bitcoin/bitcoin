<?php
require '/var/db.bitnom.inc';

class Problem extends Exception
{
    # PHP sucks!
    public function __construct($title, $message)
    {
        parent::__construct($message);
        $this->title = $title;
    }

    public function getTitle()
    {
        return $this->title;
    }
}
class Error extends Problem
{
}

# -----------------------------------------------------
# These functions are for the API
# -----------------------------------------------------
function posten($key)
{
    if (!isset($_POST[$key]))
        throw new ErrorJson(MISSING_POST_VALUE);
    return escapestr($_POST[$key]);
}
function geten($key)
{
    if (!isset($_GET[$key]))
        throw new ErrorJson(MISSING_POST_VALUE);
    return escapestr($_GET[$key]);
}

function do_queryn($query)
{
    if (!$result = mysql_query($query)) 
        throw new ErrorJson(SQL_FAILURE, array('mysql_err' => mysql_error()));
    return $result;
}
function get_rown($result)
{
    $row = mysql_fetch_array($result, MYSQL_ASSOC);
    if (!$row)
        throw new ErrorJson(RECORD_NOT_FOUND);
    return $row;
}

# -----------------------------------------------------
function do_query($query)
{
    $result = mysql_query($query) or die(mysql_error());
    return $result;
}
function get_row($result)
{
    $row = mysql_fetch_array($result, MYSQL_ASSOC);
    if (!$row)
        throw new Error('Ooops!', "Seems there's a missing value here.");
    return $row;
}

function has_results($result)
{
    if (mysql_num_rows($result) > 0)
        return true;
    else
        return false;
}

function escapestr($str)
{
    return mysql_real_escape_string(strip_tags(htmlspecialchars($str)));
}

function user_id()
{
    if (!isset($_SESSION['uid'])) {
        # grave error. should never happen and should be reported as urgent breach.
        throw new Error('Login 404', "You're not logged in. Proceed to the <a href='login.php'>login</a> form.");
    }
    return escapestr($_SESSION['uid']);
}

function post($key)
{
    if (!isset($_POST[$key]))
        throw new Error('Ooops!', "Missing posted value $key!");
    return escapestr($_POST[$key]);
}
function get($key)
{
    if (!isset($_GET[$key]))
        throw new Error('Ooops!', "Missing get value $key!");
    return escapestr($_POST[$key]);
}

