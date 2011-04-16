<?php
require '../util.php';
require '../errors.php';

try {
    $nickname = posten('nickname');
    $signature = posten('signature');
    $timestamp = posten('timestamp');
    $address = posten('address');

    $signature = base64_decode($signature);
    $data = $nickname . $address . $timestamp;

    $query = "
        SELECT
            pubkey,
            users.uid AS uid
        FROM
            users
        JOIN
            nicknames
        ON
            nicknames.uid=users.uid
        WHERE
            nickname='$nickname'
        ";
    $result = do_queryn($query);
    if (!has_results($result))
        throw new ErrorJson(RECORD_NOT_FOUND);
    $row = get_rown($result);
    $pubkey = $row['pubkey'];
    $uid = $row['uid'];
    $pubkey = openssl_get_publickey($pubkey);
    if (!$pubkey)
        throw new ErrorJson(NO_PUBKEY);

    $ok = openssl_verify($data, $signature, $pubkey, "sha512");

    if ($ok == 1) {
        $query = "
            UPDATE
                nicknames
            SET
                addr='$address'
            WHERE
                nickname='$nickname'
                AND uid='$uid'
            ";
        do_queryn($query);
        $json = array('status' => 'updated address');
        $json['new'] = $address;
        $json['timestamp'] = (int)$timestamp;
        echo json_encode($json);
    }
    else if ($ok == 0) {
        throw new ErrorJson(BAD_SIGNATURE);
    }
    else {
        throw new ErrorJson(INTERNAL_ERROR);
    }

    openssl_free_key($pubkey);
}
catch (ErrorJson $e) {
    echo $e->getMessage();
}

