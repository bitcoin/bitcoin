<?php
require '../util.php';
require '../errors.php';

try {
    $nickname = geten('nickname');
    $query = "
        SELECT
            addr
        FROM
            nicknames
        WHERE
            nickname='$nickname'
        ";
    $result = do_queryn($query);
    if (!has_results($result))
        throw new ErrorJson(RECORD_NOT_FOUND);
    $row = mysql_fetch_assoc($result);
    if (!isset($row['addr']))
        throw new ErrorJson(NO_ADDR_SET);

    $json = array('address' => $row['addr']);
    echo json_encode($json);
}
catch (ErrorJson $e) {
    echo $e->getMessage();
}

