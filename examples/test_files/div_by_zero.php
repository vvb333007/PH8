<?php

try {
    $a = 0;
//  $b =1/$a;                 // Работает как в PHP8
//  $b =1%$a;                 // Работает как в PHP8
//  $b /= $a;                 // Работает как в PHP8
 $b %= $a;                 // Работает как в PHP8
  
  echo 'Boom'.PHP_EOL;
  echo 'Boom'.PHP_EOL;
  echo 'Boom'.PHP_EOL;

} catch( Throwable $t) {

  echo 'Caught: ['.$t->getFile().':'.$t->getLine().'] '.$t->getMessage().PHP_EOL;
}

echo 'Final'.PHP_EOL;
echo 'Final'.PHP_EOL;
echo 'Final'.PHP_EOL;
echo 'Final'.PHP_EOL;









?>