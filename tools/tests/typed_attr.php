<?php

class Test {
  public $x;
  public float $zz = 1.1;
  public static bool $zzz;
  public ?string $a;
  public static int $b;
  public function func() {
    return new static;
  }
}

$cl = new Test;

$cl->a = 1;
if ($cl->a === 1) {
  var_dump($cl->a);
  die("Nullable string fails at int");
}


$cl->b = '88.7';
if ($cl->b !== 88)
  die("Typed int property fails at float");

$cl->a = null;
if ($cl->a !== null) {
  var_dump($cl->a);
  die("Typed nullable property fails at null");
}

$cl->b = null;
if ($cl->b === null)
  die("Typed non-nullable property fails at null");

echo 'TEST PASSED';
/*
$cl->x = 1;
var_dump($cl->x);

$cl->x = '88.7';
var_dump($cl->x);

$cl->x = null;
var_dump($cl->x);
*/

?>