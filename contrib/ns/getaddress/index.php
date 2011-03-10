<?php
if (!isset($_REQUEST['nickname'])) {
    die('{"error": "No nickname received."}');
}
else {
    require '../db.php';
    # nicknames are case insensitive to limit impersonation attacks
    $nickname = strtolower(escapestr($_REQUEST['nickname']));

    $query = "SELECT address FROM lookup WHERE nickname='$nickname';";
    $result = do_query($query);
    if (has_results($result)) {
        $row = mysql_fetch_array($result);
        if (!isset($row['address']))
            die('{"error": "Internal error."}');
        $address = $row['address'];
        echo '{"address": "'.$address.'"}';
    }
    else {
        die('{"error": "No such nickname exists."}');
    }
}
?>

