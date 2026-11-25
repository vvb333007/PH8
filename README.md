### PH8 : A further development of PH7 PHP Engine. 

**Work in progress** [A roadmap](https://github.com/vvb333007/PH7-ESP32/blob/master/TODO.md)

**Comparision: PH8 vs ZendPHP**: [current status](https://github.com/vvb333007/PH8/blob/master/docs/Language.md)

**Сравнение PH8 и ZendPHP**: [состояние дел](https://github.com/vvb333007/PH8/blob/master/docs/Language_RU.md)


Compiles and runs on ESP32-family, Linux and Windows (Cygwin).
Memory footprint: requires about 100KiB of memory for the Engine plus ~180 KiB per virtual machine;


### Compile and run: Windows (Cygwin) or Linux

  * Make sure you have Cygwin installed
  * Clone the PH8 repository
  * Go to PH8/src/examples/linux_windows_example/
  * execute `make`. You should get `inter.exe` or `inter` (on Linux)
  * Optional: Copy `PH8/Libraries/ext` to the `inter.exe` directory - this is an extension library (Arduino API, ESP-IDF API, SQLite3 API)
  * Optional: Copy  `PH8/examples/test_files/*.php` to the `inter.exe` directory - these are examples
  * Execute `inter.exe` with a PHP file as an argument: 'inter.exe SCRIPTAME.PHP'


### Partial list of what has been done so far (PH8 vs PH7):

1. Buggy function overloading mechanism is fixed; score-based resolution is added; mixed typed and untyped function arguments can be used in overloaded functions; functions can not be declared twice;
2. Exceptions can be thrown by the engine in some cases (e.g. division by zero, unknown function/method call and so on); new Exception/Error classes; Exceptions can be thrown by builtins as well
3. HTTP parser was rewritten: multipart, chunked, continue are all supported
4. Constants fixed: complex connstant initializers are evaluated once and cached; Constants can not be redeclared/redefined
5. Memory bugs: numerous memory violation bugs (wrong sizeof() usage); 64/32 bit related bugs
   Program logic bugs (parser, vm, vfs);
   Compiler bugs (too many and too specific to list)
6. Typed class properties implemented
7. Function return types; `:never` return type; `callable` type;
8. Enums
9. Namespaces and Anonymous classes
10. Traits
11. Class property promotion
12. Operators: ?-> , ??, ??=, ?:, <=>  (nullsafe, null coalesce/store, elvis and a spaceship)
13. Regexp support (preg_ API)
14. SQLite3 support (functions and classes are both supported)
15. match() language construct
16. Builtins for FreeRTOS and Arduino
17. Built-in HTTP server
18. New language constructs (extensions to the language, like `compile`,`compile_once`, `(reliable)`
19. Magic methods were fixed (__call, __callStatic, __get, __invoke): now can accept args and return values
20. VM fixes: operand stack cleanup, bugs in SNE, SE opcodes, bugs in array load/store opcodes
21. Builtins: numerous fixes
22. Arrays: bugs in $string[] access; Short array syntax `[]` implemented
21. Destructuring: short destructuring syntax implemented


