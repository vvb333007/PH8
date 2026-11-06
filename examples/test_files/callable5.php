<?php

function test() :callable {
  return 'echo';
}

(test())("z");
?>