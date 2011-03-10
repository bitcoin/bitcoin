<?php
if (!isset($_POST['nickname'])) {
    die('{"error": "No nickname received."}');
}
else if (!isset($_POST['password'])) {
    die('{"error": "No password received."}');
}
else if (!isset($_POST['newpassword'])) {
    die('{"error": "No new password received"}');
}
else {
    require 'db.php';
    $nickname = strtolower(escapestr($_POST['nickname']));
    $oldpasshash = hash('sha512', $_POST['password']);
    $newpasshash = hash('sha512', $_POST['newpassword']);

    $query = "SELECT passhash FROM lookup WHERE nickname='$nickname';";
    $result = do_query($query);
    if (has_results($result)) {
        $row = mysql_fetch_array($result);
        if (isset($row['passhash']) && $oldpasshash == $row['passhash']) {
            $query = "UPDATE lookup SET passhash='$newpasshash' WHERE nickname='$nickname' AND passhash='$oldpasshash';";
            do_query($query);
            echo '{"status": "Password changed."}';
        }
        else {
            die('{"error": "Incorrect password."}');
        }
    }
    else {
        die('{"error": "No such nickname exists."}');
    }
}
?>

