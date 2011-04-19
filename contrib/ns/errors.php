<?php

const MISSING_POST_VALUE = 0;
const SQL_FAILURE = 1;
const RECORD_NOT_FOUND = 404;
const NO_PUBKEY = 2;
const BAD_SIGNATURE = 3;
const INTERNAL_ERROR = 4;
const NO_ADDR_SET = 5;

function error_json($errcode, $params=array())
{
    $json = array('errcode' => $errcode);
    switch ($errcode) {
        case MISSING_POST_VALUE:
            $json['error'] = 'Missing POST value.';
            break;

        case SQL_FAILURE:
            $json['error'] = 'SQL query failed.';
            break;

        case RECORD_NOT_FOUND:
            $json['error'] = 'Record not found.';
            break;

        case NO_PUBKEY:
            $json['error'] = 'Public key does not exist.';
            break;

        case BAD_SIGNATURE:
            $json['error'] = 'Bad signature.';
            break;

        case INTERNAL_ERROR:
            $json['error'] = 'Internal error occurred.';
            break;

        case NO_ADDR_SET:
            $json['error'] = 'No address set for this nickname.';
            break;

        default:
            $json['error'] = 'Unknown error code.';
            break;
    }
    return json_encode($json);
}

class ErrorJson extends Exception
{
    public function __construct($errcode, $params=array())
    {
        parent::__construct(error_json($errcode, $params));
    }
}

