<?php
$z = 1;

function &ffn() {
  global $z;
  return $z;
}

//PH8: function returning references can be used as array initializers
//ZendPHP: syntax error

$a = array( &ffn() );

$a[0] = 10;

var_dump($z);  // displays 10

?>