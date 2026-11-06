<?php

class Proxy {

    public function __invoke($a, $b):string {
      return 'Return value '.$a.$b;
    }
}

$z = (new Proxy)('AA', 'BB');
var_dump($z);

?>
