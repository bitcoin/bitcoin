<?php
if (!isset($_POST['nickname'])) {
    die('{"error": "No nickname received."}');
}
else if (!isset($_POST['password'])) {
    die('{"error": "No password received."}');
}
else if (!isset($_POST['address'])) {
    die('{"error": "No address received"}');
}
else {
    require '../db.php';
    # nicknames are case insensitive to limit impersonation attacks
    $nickname = strtolower(escapestr($_POST['nickname']));
    $address = $_POST['address'];
    # bitcoin address can only be alphanumeric so no need to strip them...
    $address = preg_replace("/[^a-zA-Z0-9]/", "", $address);
    $passhash = hash('sha512', $_POST['password']);

    $query = "SELECT passhash FROM lookup WHERE nickname='$nickname';";
    $result = do_query($query);
    if (has_results($result)) {
        $row = mysql_fetch_array($result);
        if (isset($row['passhash']) && $passhash == $row['passhash']) {
            $query = "UPDATE lookup SET address='$address' WHERE nickname='$nickname' AND passhash='$passhash';";
            do_query($query);
            echo '{"status": "Address changed.", "address": "'.$address.'"}';
        }
        else {
            die('{"error": "Incorrect password."}');
        }
    }
    else {
        $query = "INSERT INTO lookup(nickname, passhash, address) VALUES ('$nickname', '$passhash', '$address');";
        do_query($query);
        echo '{"status": "New user created.", "address": "'.$address.'"}';
    }
}
?>

