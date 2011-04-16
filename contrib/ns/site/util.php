<?php

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

function cleanup_string($val)
{
    $val = preg_replace('/[^A-Za-z0-9 .]/', '', $val);
    return mysql_real_escape_string($val);
}

function user_id()
{
    if (!isset($_SESSION['uid'])) {
        # grave error. should never happen and should be reported as urgent breach.
        throw new Error('Login 404', "You're not logged in. Proceed to the <a href='login.php'>login</a> form.");
    }
    return cleanup_string($_SESSION['uid']);
}

function post($key)
{
    if (!isset($_POST[$key]))
        throw new Error('Ooops!', "Missing posted value $key!");
    return cleanup_string($_POST[$key]);
}
function get($key)
{
    if (!isset($_GET[$key]))
        throw new Error('Ooops!', "Missing get value $key!");
    return cleanup_string($_GET[$key]);
}

