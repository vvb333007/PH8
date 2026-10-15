<?php


$p = 14;


try {
$p->method();
  echo 'FAILED';
} catch(Throwable $t) {
  echo $t->getMessage();
}
echo 'PASSED';

?>