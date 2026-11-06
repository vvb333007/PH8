<?php

$a = [1, 2, 3];
$k = array_keys($a, 2);
var_dump($k);   // ожидаем [1]
var_dump($a);   // ожидаем [1,2,3]; по моему чтению кода должно быть [null,null,null]


$a = [0, "abc", "ABC"];
var_dump(array_keys($a, "abc"));   // ожидаем [0, 1]
var_dump($a);                      // ожидаем исходный массив
?>
