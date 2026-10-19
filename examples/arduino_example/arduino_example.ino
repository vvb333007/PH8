/* Пример использования PHP движка PH7 в своем коде в Ардуино ИДЕ, на ESP32
 * ВАЖНО: tar -cf filesystem.tar scripts; потом прошейте TAR архив в свою ESPшку по 
 * смещению 0x310000 (см. partitions.csv). Установите в настройках IDE (Partition scheme->Custom)
 * 
 */

#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tarfs.h" // Arduino Library Manager: "TARFS"
#include "ph7.h"   // Arduino Library Manager: "PH7-ESP32" (not yet :)


/*
 * Когда виртуальная машина выводит что-либо (вызывая, echo или printf ), весь ее вывод попадает в эту функцию.
 * Все, что делает эта функция - перенаправляет весь вход на экран (на stdout)
 */
static int output_callback(const void *pOutput,unsigned int nOutputLen,void *pUserData /* Unused */) {
  Serial.printf("%.*s",nOutputLen, pOutput );
  return PH7_OK;
}


/* Запустить PHP скрипт с таким-то именем. Скрипты должны быть расположены на смонтированной файловой 
 * системе TARFS, т.к. другие файловые системы не поддерживают вызов mmap(), которые необходим для работы PH7
 *
 * Показан полный цикл. В реальном коде создавать\уничтожать Engine каждый раз нет необходимости.
 * На одной Engine может висеть несколько виртуальных машин, в функции-примере ниже используется лишь 
 * одна машина
 */
int engine_test(const char *path) {

  
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

  // Можно компилировать файлики. Файлик может быть любого размера (лишь бы влазил во флешку)
  // при компиляции файл в память _не_ загружается.
  //
  rc = ph7_compile_file(pEngine, path , &pVm, 0);

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

  // монтируем файловую систему из раздела "tarfs" флешки. Там должен быть прошит TAR архив с примерами
  // (создать архив: cd examples; tar -cf tarfs.tar examples)
  //
  tarfs_init();
  tarfs_mount("tarfs",0,0,0);
}

void loop() {

  delay(1000);  
  engine_test("/scripts/hello_world.php");
}





#if 0
/* Пример использования PHP движка PH7 в своем коде в Ардуино ИДЕ, на ESP32
 * ВАЖНО: tar -cf filesystem.tar scripts; потом прошейте TAR архив в свою ESPшку по 
 * смещению 0x310000 (см. partitions.csv). Установите в настройках IDE (Partition scheme->Custom)
 * 
 */

#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tarfs.h" // Arduino Library Manager: "TARFS"
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
  return write(1,pOutput,nOutputLen) < 0 ? PH7_ABORT : PH7_OK;
}


/* Запустить PHP скрипт с таким-то именем. Скрипты должны быть расположены на смонтированной файловой 
 * системе TARFS, т.к. другие файловые системы не поддерживают вызов mmap(), которые необходим для работы PH7
 *
 * Показан полный цикл. В реальном коде создавать\уничтожать Engine каждый раз нет необходимости.
 * На одной Engine может висеть несколько виртуальных машин, в функции-примере ниже используется лишь 
 * одна машина
 */
int engine_test(const char *path) {

  
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

  // Можно компилировать файлики. Файлик может быть любого размера (лишь бы влазил во флешку)
  // при компиляции файл в память _не_ загружается.
  //
 // rc = ph7_compile_file(pEngine, path , &pVm, 0);
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

  // монтируем файловую систему из раздела "tarfs" флешки. Там должен быть прошит TAR архив с примерами
  // (создать архив: cd examples; tar -cf tarfs.tar examples)
  //
  //tarfs_init();
  //tarfs_mount("tarfs",0,0,0);
}

void loop() {

  delay(1000);  
  engine_test("/scripts/hello_world.php");
}

#endif