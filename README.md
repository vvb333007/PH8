### PH8: picking up where PH7 left off

**Work in progress** [A roadmap](https://github.com/vvb333007/PH7-ESP32/blob/master/TODO.md)

**Comparision: PH8 vs ZendPHP**: [current status](https://github.com/vvb333007/PH8/blob/master/docs/Language.md)

**Сравнение PH8 и ZendPHP**: [состояние дел](https://github.com/vvb333007/PH8/blob/master/docs/Language_RU.md)


### Goal of the project

PH8 aims to be a multithreaded PHP engine for embedded and IoT: it supports modern PHP
syntax and gives scripts direct access to the hardware through the Arduino and ESP-IDF APIs

### Compile and run: Windows (Cygwin) or Linux

1. On Windows, install [Cygwin](https://www.cygwin.com/) with the `gcc-core` and
   `make` packages. On Linux, you only need `gcc` and `make`.
2. Clone the PH8 repository.
3. Go to `PH8/examples/linux_windows_example/`.
4. Run `make`. This builds the PH8 library and the interpreter: `inter.exe` on
   Windows, `inter` on Linux.
5. *Optional:* copy `PH8/Libraries/ext` to the interpreter's directory. This is
   the extension library (Arduino API, ESP-IDF API, SQLite3 API).
6. *Optional:* copy `PH8/examples/test_files/*.php` to the interpreter's
   directory. These are example scripts.
7. Run the interpreter with a PHP file as its argument:

       ./inter.exe script.php      # Windows (Cygwin)
       ./inter script.php          # Linux

### Partial list of what has been done so far (PH8 vs PH7)

#### Language

- Typed class properties
- Function return types, including `:never`; the `callable` type
- Enums
- Arrow functions
- Namespaces and anonymous classes
- Traits
- Constructor property promotion
- `match()` language construct
- New operators: `?->`, `??`, `??=`, `?:`, `<=>` (nullsafe, null coalescing and
  null coalescing assignment, Elvis, spaceship)
- Short array syntax `[]`
- Short destructuring syntax `[$a, $b] = ...`
- PH8 extensions to the language: `compile`, `compile_once`, `(reliable)`

#### Engine

- Function overloading reworked: the buggy mechanism is fixed, score-based
  resolution is added, overloaded functions can mix typed and untyped arguments,
  a function can no longer be declared twice
- Exceptions: the engine itself throws in some cases (division by zero, call to
  an unknown function or method, etc.); new `Exception`/`Error` classes;
  builtins can throw exceptions as well
- Constants: complex initializers are evaluated once and cached; constants can
  no longer be redeclared or redefined
- Magic methods `__call`, `__callStatic`, `__get`, `__invoke` fixed: they now
  accept arguments and return values

#### Bug fixes

- Memory: numerous memory violations (wrong `sizeof()` usage), 64/32-bit issues
- VM: operand stack cleanup; bugs in the `SE`/`SNE` opcodes and in the array
  load/store opcodes
- Compiler: too many and too specific to list
- Program logic bugs in the parser, VM and VFS
- Arrays: `$string[]` access
- Builtins: numerous fixes

#### Libraries and hardware

- Regular expressions (`preg_*` API)
- SQLite3: both the function API and the classes
- Built-in HTTP server; the HTTP parser was rewritten and supports multipart,
  chunked transfer encoding and `Expect: 100-continue`
- Builtins for FreeRTOS, Arduino and ESP-IDF
