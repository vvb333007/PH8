<?php

$a = [1, 2, 3];
$k = array_keys($a, 2);
var_dump($k);   // PH8: ожидаем [1]                  PH7: [1]
var_dump($a);   // PH8: ожидаем [1,2,3];             PH7 [null, null, null]


$a = [0, "abc", "ABC"];
var_dump(array_keys($a, "abc"));   // PH8: ожидаем [0, 1]            PH7: [0,1,2]
var_dump($a);                      // PH8: ожидаем исходный массив   PH7: [null, null, null]
?>
