<?php





try {
  echo stdClass::Member();  
  echo 'FAILED';
} catch(Throwable $t) {
  echo $t->getMessage();
}
echo 'PASSED';

?>