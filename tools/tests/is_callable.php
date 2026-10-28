<?php
/*
Expected output:

Testing un-typed callback
Hello World!
Hello, guest
Expecting a callable type
Testing typed callback
Hello World!
Hello, guest
Testing class as a callable

/* 'callable' as an argument type enforces strict argument check: VM stops if argument is not callable
 *
 */
function test(callable $callback = function() { echo "Hello World!\n";}) {

  $callback();
}

/* 'mixed' type (or no-type) allows anything to be passed without checking. 
 *  Type is checked later by 'is_callable()'
 */
function test2($callback = function() { echo "Hello World!\n";}) {

  if (!is_callable($callback)) {
    echo "Expecting a callable type\n";
    return ;
  }
  $callback();
}

echo "Testing un-typed callback\n";

test2();
test2(  ($z = function () { echo "Hello, guest\n"; })  );
test2(11);

echo "Testing typed callback\n";

test();
test($z);


class Test {

  public function oke($x,$y,$z) {
  }

  public function __invoke($x,$y,$z) {
    var_dump($x);
    var_dump($y);
    var_dump($z);
  }
}

echo "Testing class as a callable\n";

$z = new stdClass(1);
if (is_callable($z))
  die("Failed; Class stdClass is NOT expected to be a callable");

$z = new Test;
if (!is_callable($z))
  die("Failed; Class is expected to be a callable");

$b = [$z, 'oke'];
if (!is_callable($b))
  die("Failed; Array is expected to be a callable");

$b = [$z, 'chmoke'];
if (is_callable($b))
  die("Failed; Array is NOT expected to be a callable");



//test(11);
//echo 'This text should not be printed, because test(11) caused VM stop';

?>
