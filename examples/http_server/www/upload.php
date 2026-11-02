<?php

$vars = [
    '_GET'     => $_GET,
    '_POST'    => $_POST,
    '_FILES'   => $_FILES,
    '_COOKIE'  => $_COOKIE,
    '_REQUEST' => $_REQUEST,
    '_SERVER'  => $_SERVER,
];

//if (session_status() === PHP_SESSION_ACTIVE) {
//    $vars['_SESSION'] = $_SESSION;
//}

echo '<pre>';

foreach ($vars as $name => $value) {
    echo "===== \$$name =====\n";
    print_r($value);
    echo "\n";
}

echo '</pre>';

