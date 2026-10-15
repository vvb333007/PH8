<?php

$a = 12;
$b = 0;
try {
$c = $a % $b;
} catch( Error $err) {
  echo $err->getMessage().PHP_EOL;
}
echo 'Result is:'.$c.PHP_EOL;
//$c = $a % $b;
//echo 'This code is not reached'.$c.PHP_EOL;
?>