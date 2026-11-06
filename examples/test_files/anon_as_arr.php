<?php

$a = [
     'add' => fn($x,$y) => $x+$y,
     'mul' => fn($x,$y) => $x*$y,
     'div' => fn($x,$y) => $x/$y,
];
/* Extract the anonymous function performing an addition */
$add = $a['add'];
/* Invoke */
echo $add(100,200); //Output: 300
/* Invoke our anonymous function directly by dereferencing the array */
echo $a['add'](50,20).PHP_EOL; //Addition: Output 70
echo $a['mul'](10,20).PHP_EOL; //Multiplication: Output 200
echo $a['div'](50,2); //Division: Output 25

?>
