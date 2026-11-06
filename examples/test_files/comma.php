<?php

$a = 25,$b = $a << 1 ,test();

/* Output the value of $a and $b */
function test(){
  global $a,$b;
  echo "\$a = $a \$b= $b\n"; /* You should see: $a = 25 $b = 50*/
}
?>
