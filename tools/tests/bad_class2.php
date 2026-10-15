<?php

$passed = 0;

class Test {
  public static $A = 1 / 0;
}


try {
  $passed++;
  echo 'STEP 1 of 3'.PHP_EOL;
  echo Test::A;
  die('Exception failed');
} catch (Error $r) {
  global $passed;
  echo 'Caught: ', $r->getMessage() , PHP_EOL;
  echo 'STEP 2 of 3'.PHP_EOL;
  $passed++;
}

$a = new Test;
if ($a != null)
  die('operator new() failed');
$passed++;

echo 'STEP 3 of 3'.PHP_EOL;
if ($passed == 3)
  echo 'TEST PASSED'.PHP_EOL;
else
  die('TEST FAILED');


?>