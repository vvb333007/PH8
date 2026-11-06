<?php


$m = []; // IMPORTANT! $m array must be created prior to call. The reason for that
         // is to hint the compiler to pass by reference!


echo "Test 1: simple match\n";

$n = preg_match('/hello/', 'hello world', $m);
var_dump($n);
var_dump($m);

echo "\nTest 2: no match\n";

$n = preg_match('/xyz/', 'hello world', $m);
var_dump($n);
var_dump($m);

echo "\nTest 3: capture groups\n";

$n = preg_match('/([a-z]+) ([0-9]+)/', 'item 123', $m);
var_dump($n);
var_dump($m);

echo "\nTest 4: match at the end\n";

$n = preg_match('/world$/', 'hello world', $m);
var_dump($n);
var_dump($m);

echo "\nTest 5: case-insensitive match\n";

$n = preg_match('/hello/i', 'HELLO', $m);
var_dump($n);
var_dump($m);

echo "\nTest 6: repeated characters\n";

$n = preg_match('/(a+)(b+)/', 'xaaabbz', $m);
var_dump($n);
var_dump($m);






$subject = "abcdef";
$pattern = '/^def/';
preg_match($pattern, $subject, $m, 256, 3);
print_r($m);

$subject = "abcdef";
$pattern = '/^def/';
preg_match($pattern, substr($subject,3), $m, 256 /*PREG_OFFSET_CAPTURE*/);
print_r($m);

?>
