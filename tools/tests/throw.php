<?php

function test() {
  $a = 0;
  $b = 1/$a;                 // DivisionByZero exception

}

function test2() {
  test7();                   // UndefinedFunction exception
}

function test3(): callable {
  return 1;
}




try {
    $a = 0;
//  throw new Error("A message", 666);     // Работает как в PHP8
   (test3())();
//  $b =1/$a;                 // Работает как в PHP8
//  $b /= $a;
//  test();                 // Работает как в PHP8
//  test2();                 // Работает как в PHP8
//  test7();                   // Работает как в PHP8


  
  echo 'Boom'.PHP_EOL;
  echo 'Boom'.PHP_EOL;
  echo 'Boom'.PHP_EOL;

} catch( Throwable $t) {

  echo $t; //'Caught: ['.$t->getFile().':'.$t->getLine().'] '.$t->getMessage().PHP_EOL;
}

echo 'Final'.PHP_EOL;
echo 'Final'.PHP_EOL;
echo 'Final'.PHP_EOL;
echo 'Final'.PHP_EOL;









?>