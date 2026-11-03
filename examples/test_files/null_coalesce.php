<?php

$j = 7;

// 7
$a = $v ?? $d ?? $j ?? 99;
var_dump($a);

// Always FALSE: 
// Executed from left to right: $a||$b yields boolean type --> '??' operator fails
$z = $aa || $bb ?? $a;   
var_dump($z);

// TRUE: 
// Executed from left to right: $bb ?? $a yields $a which is then ORed with TRUE
$z = $aa || ($bb ?? $a);   
var_dump($z);


var_dump(5 ?? 0 || 0);
?>