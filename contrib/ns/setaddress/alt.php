<?php
require '/var/db.bitnom.inc';

function cleanup_string($str)
{
    return mysql_real_escape_string($str);
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

$pubkey = "-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA9X6CpkoqdPouFzUoWPFq
xSCpXvR34Z5cQM7LX5qlRR3yHwT8N7s+U6AerEP6v3usPFqL92Q96/kDVwEeKOCe
pEZgjEVDP8t+wDXs+SfCgn5sG/ueAgzYnPTdQVF5vuFuBQlhhhlnyjoBRvzBKhWF
jU/fyLD1sbYRQFv5vLujwZ0HIoPlDBsmvREX5zsujQxNz8IPrULhPxN/SB4lOwYM
UP++msYgbWbJS9oOk8rNBSCAu8N/VxuayJNx/8ENRk/cAIFjL57EvsJyA1gwE7Dk
MlILZYBQs+eSC1H0bsGqhsUTj9OT2YREoY00uNBnGdCl73h24tyYi3SaefbDREpv
GQIDAQAB
-----END PUBLIC KEY-----";

#$nickname = post('nickname');
$nickname = $_POST['nickname'];
#$signature = post('signature');
$signature = $_POST['signature'];
#$timestamp = post('timestamp');
$timestamp = $_POST['timestamp'];
$address = post('address');
#$address = $_POST['address'];

$signature = base64_decode($signature);
$pubkey = openssl_get_publickey($pubkey);
$data = $nickname . $address . $timestamp;

$ok = openssl_verify($data, $signature, $pubkey, "sha512");

if ($ok == 1) {
    echo '{"status": "good", "address": "'.$address.'"}';
}
else if ($ok == 0) {
    echo '{"status": "bad", "address": "'.$address.'"}';
}
else {
    echo '{"status": "ugly"}';
}

openssl_free_key($pubkey);
