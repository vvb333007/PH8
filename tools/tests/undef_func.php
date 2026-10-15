<?php

try {
  hello(10);
} catch(Throwable $ex) {
    echo $ex->getMessage() . PHP_EOL;
}

echo 'Still alive!';

//hello(10);

//echo 'Not reached';

?>