<?php

// class name overload collision:
// function arguments (having a class type) inject class name to the function signature
// E.g.
// function a(int, int, string, mixed, MyClass, int)  --> Signature: 'iismMyClassi'
//
// Problem arises from the fact that class names may be a substrings of each other
// PH7 does not see Test2 and Test arguments as different types
//
// 


class Test {
}

class Test2 {
}


function foo( Test &$a) {
  echo "Hello, lucky guy! You are running PH8!\n";
  die();
}

function foo( Test2 &$a) {
  echo "Hello, lucky guy! You are running PH7!\n";
  die();
}

function foo(mixed $a) {
  echo "Mixed\n";
}

$Test = new Test();
$Test2 = new Test2();

// foo($Test, 1.1);  // Error: no matching overload

foo(666);     
foo($Test);
foo($Test2);





?>