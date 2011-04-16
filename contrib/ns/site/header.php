<!DOCTYPE html>
<?php
error_reporting(E_ALL|E_STRICT);
ini_set('display_errors', '1');
session_start();

function write_header()
{
?>
<html>
<head>
    <title>Bitnom</title>
<style>
input[type=text], textarea {
    display: block;
}
textarea {
    width: 600px;
    height: 300px;
}
pre {
    white-space: pre-wrap;
}
#prop
{
    font-family:"Trebuchet MS", Arial, Helvetica, sans-serif;
    width:100%;
    border-collapse:collapse;
}
#prop td, #prop th 
{
    font-size:1.2em;
    border:1px solid #98bf21;
    padding:3px 7px 2px 7px;
}
#prop th 
{
    font-size:1.4em;
    text-align:left;
    padding-top:5px;
    padding-bottom:4px;
    background-color:#A7C942;
    color:#fff;
}
#prop tr.alt td 
{
    color:#000;
    background-color:#EAF2D3;
}

</style>
</head>
<body>
    <div id='content'>
<?php }

function write_footer()
{
?>
    </div>
</body>
</html>
<?php
}

