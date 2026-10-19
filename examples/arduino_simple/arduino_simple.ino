
#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ph7.h"   // Arduino Library Manager: "PH8" (not yet :)

char * const zSource = "\r\n"
"<?php\r\n"
" echo 'Welcome guest'.PHP_EOL;\r\n"
" echo 'Current system time is: '.date('Y-m-d H:i:s').PHP_EOL;\r\n"
" echo 'and you are running '.php_uname();\r\n"
"?>\r\n";

/*
 * Когда виртуальная машина выводит что-либо (вызывая, echo или printf ), весь ее вывод попадает в эту функцию.
 * Все, что делает эта функция - перенаправляет весь вход на экран (на stdout)
 */
static int output_callback(const void *pOutput,unsigned int nOutputLen,void *pUserData /* Unused */) {
  Serial.printf("%.*s",nOutputLen, pOutput );
  return PH7_OK;
}


int engine_test() {

  
  ph7 *pEngine; /* PH7 engine */
  ph7_vm *pVm;  /* Compiled PHP program */
  int rc;

  rc = ph7_init(&pEngine);

  if( rc != PH7_OK ){
    Serial.printf("Error while allocating a new PH7 engine instance\r\n");
    return -1;
  }

  // Ошибки -> на экран
  ph7_config(pEngine,PH7_CONFIG_ERR_OUTPUT, output_callback, 0); 

  rc = ph7_compile(pEngine, zSource, strlen(zSource), &pVm);

  if( rc == PH7_OK ) { 

    // Настроим нашу VM:
    // Сообщения -> на экран
    // Если PHP скрипту нужна tmp директория, то вот она:
    ph7_config(pEngine,PH7_VM_CONFIG_TEMPDIR,"/ffat/tmp" );
    ph7_vm_config(pVm, PH7_VM_CONFIG_OUTPUT, output_callback, 0 );

    
    // Ну, вроде все. Запускаем скомпилированный код, без параметров. 
    // Параметры тоже можно передать, но в примере лень писать.
    ph7_vm_exec(pVm,0);

    // Уничтожаем виртуальную машину, Уничтожаем сам движок
    ph7_vm_release(pVm);
    ph7_release(pEngine);

    return 0;
  }

  return -1;
}


void setup() {

  Serial.begin(115200);
}

void loop() {

  delay(1000);  
  engine_test();
}
