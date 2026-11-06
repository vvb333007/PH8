<?php

const ARR = ['a', 'b'];
const A = new stdClass(1);
const NUM = 1 + 2 + $z;
const RANDOM = rand_str(16);
const FUNC = function($a) { echo $a, PHP_EOL; };
const IX = $var ? 1 : 2;

//ARR[1] = 1; // Compile-time error

$z = A;
var_dump($z === A); // TRUE, reference to the same object
var_dump(new stdClass(1) === A); // FALSE, two different objects
var_dump(FUNC); // "lambda_1"

//FUNC('Hello, World'); // Unknown function call
$z = FUNC;
$z('Hello, World!');

var_dump(IX);

?>
