<?php


$a = [1,2,3, 4];
var_dump($a);

$b = [[1,2],[3,4]];
var_dump($b);

$c = ['x'=>[1=>2], 'y'=>3];
var_dump($c);

echo [1,2,3][1];                  


//echo count([1,2], 3);           
//echo count([1,2] );             

$d = [1] + [5,6]; //[1,6]
var_dump($d);

echo $a[0] . $b[1][0];             


function &foo() : array {
  return [4,5,6];
}

$s = "x"; echo $s[0];

echo foo()[0];
echo ($s)[0];

?>
