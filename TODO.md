# TODO

Text marked as ~~this~~ means that it is done.

## Milestones

VMs can be cloned (shared code, private data), required for fork().
Refactor memory subsystem. Now it is a malloc() for everything
PH7_VmCallUserFunction must be optimized (no mallocs).

---

### 1. Interrupt subsystem

Register PHP functions as interrupt handlers and provide event-based dispatch.
C code handles interrupts and notifies the PHP engine.
The PHP engine calls the registered PHP interrupt handler ().

PHP interrupts are soft interrupts, allowing arbitrary, potentially heavy code to be executed from the interrupt handler.

Supported interrupt types: GPIO interrupts, Software Timer interrupts

---

### 2. VM Yield subsystem

* ~~Ability to pause and resume any VM regardless of its current state~~
* Ability to execute a VM for a given number of opcodes

---

### 3. ESP-IDF bindings

~~Implement a selected subset of ESP-IDF APIs, starting with the GPIO API.~~

Implement raw access to MMIO registers.

WiFi bindings, both native and hosted.

---

### 4. Arduino Core bindings and SLAC ("Standard Library of Arduino Classes")

~~An analogue of the Arduino Core `.cpp` library, implemented in PHP wherever possible.~~
Standard Arduino Core classes: HardwareSerial, SPI, Wire, FS, Print, String and so on

### 5. Namespaces

~~Implement `namespace` and `use` language constructs. Resolve names at compile time~~

Fallback for builtin functions

### 5. Traits

~~Implement traits using existing infrastucture to compile class. Same as with 'enum'. Compiled methods
are then injected to a class on MountClass;~~

Extended `use {...}` syntax; (may be hard to implement. Do we really need it? all these "insteadof" ?)

### 6. Reliable values

~~Implement MEMOBJ_UNREL - a flag saying that the value was converted but resilt is unreliable.~~

~~Implement MEMOBJ_UNREL for suspicous conversions like array to a scalar or object to a scalar~~

MEMOBJ_UNREL propagation through arithmetics

Introduce `__toArray` so objects properties can be serialized via typecast; if there were private/protected properties then conversion considered unreliable: these properties are NOT serialized
Parent properies are not serialized

Refactor (array) and (object) casts to use `__toArray`

### 7. Exceptions

~~Rename `Exception` to `Throwable`, and add `Exception`, `Error`, `TypeError`, `ArgumentCountError`, `ArithmeticError`, `DivisionByZeroError`~~

~~Replace `instanceof(Exception)` in `throw` with `Throwable`~~

~~Go throuhg error messages table, and replace them with `VmThrowException` calls~~

Verify failure branches of VmByteCodeExec by provoking the exception; 

Verify return code propagation of VmByteCodeExec, especially in VmLocalExec use

~~Fix recursion depth limiter~~

### 8. Types and overloading

  Nullable types in stdlib signatures (`?string`, `?Throwable`) or null will not pass strict checks: null will never match ?type signature when overloading.
  This is related to how function signatures are built: only type ID is stored, ? is ignored. I.e. function test(int $a, string $b, ?string $c)
  will get signature of "iss". Idea: use upper/lower case letters to distinguish between normal and nullable type.

  "No matching function" message should print argument types, and the list of candidates

  Constructor by class name - find places where it is'nt handled (`new`, `parent::`, `method_exists`)

  Union types are not supported: they are conflicting with overloading mechanism; they are conflicting with typecasting and type-guarantee mechanisms of PH8


## 9. VM shutdown

  ~~Call `__destruct` for all live objects on VM exit (globals are not destructed right now on script DONE), before function and class tables are freed~~

~~Decide what `OP_HALT` does with destructors, and write it down~~

~~Objects with a cyclic references never reach zero refcount, need a list of all live objects~~


## 10. RegEx engine

~~Implement `preg_..` style API~~

## 11. SQL

~~Import SQLite3 into PH8 codebase, make ESP32 port~~

~~Implement procedural sqlite API for PH8. It will then be used to implement SQLite3 class~~

Replace native SQLite3 memory backand with VM's memory backend

Review Trilogy source code and decide how it can be ported to mbedTLS API


## 12. HTTP(S) server

~~Implement HTTP server for static and dynamic content. Support `100-Continue`, multipart/form-data, PHP variables, query parsing~~

Implement HTTPS using mbedTLS

Implement WebSockets; Use PH8 model of websockets: script receives STDIN/STDOUT redirected via ws:// framing 

Implement SSE: basically we just need to add extra headers processing. Script keeps connection  open until client is alive;


---

## Smaller tasks


### 0. Arrays

Implement: 

`array_column()`  PHP5.5

`array_key_first()` PHP7.3

`array_key_last()`  PHP7.3

`array_replace_recursive()`

`array_merge_recursive()`

`array_count_values()`

`array_change_key_case()`

Refactor `(object)` cast of arrays: instead of wrapping up arrays to stdClass we have to create a new class with given properties in runtime

### Peoperty promotion

~~Implement constructor property promotion~~

### Magic methods

~~__invoke() does not receive any arguements right now. __call and __callStatic do not receive method name~~

implement __terminate() magic method
implement __oom() magic method
implement __toArray() magic method


### 1. '?:' Elvis operator support

Implement `$a = $b ?: $c;`


### 2. '<=>' Saceship operator

~~Implement `$a = $b <=> $c;`~~




### 2. User constants caching:


~~PH7 executes bytecode each time const value is referenced:~~
```
const A = 1+1+1+1+1; // <--- bytecode
const B = A+A+A+A+A; // <-- bytecode; every 'A' is also a bytecode
```

~~results in 25 calls to VmLocalExec()! Plus to that, constants like~~

```
const A = rand_str(5);
```

~~change their values each time they are referenced.~~

~~TODO: Execute bytecode once and cache result.~~


### 3. Constants as arrays:


~~PH7 does not support referencing constants as array elements:~~

```
const A = [1,2,3];
echo A[1]; // PH7 throws an error here
```
~~TODO: add support~~


### 4. __invoke(), __call(), __callStatic(), __get()

~~Currently do not return any values. That must be fixed ASAP; use `__toString()` as a template~~

### 5.  __call() and  __callStatic()

~~Currently do not receive called method name.~~



### 7. Background PHP services via FreeRTOS tasks

Start a service from P2HP:

```php
$error = php_service_start('Service Name'); // can be called multiple times
php_service_stop('Service Name');           // only once
```

---

### 8. IPC instead of the FreeRTOS Task Notification API

Allow VMs to communicate with each other.
The main use case is communication with PHP services (standalone background PHP processes).

Implement VM-to-VM IPC using FreeRTOS queues.
Use FreeRTOS Task Notifications for lightweight VM wake/sleep synchronization.

---

### 9. C++-style constructors

Not always checked for existence. Should we patch a constructor lookup code?



### 12. `match` keyword

~~Implement match() language construvt; Use existing switch() processing code~~

```php
    return match ($day) {
        1, 7       => 'Weekend',
        2, 3, 4, 5, 6 => 'Working day',
        default    => 'Invalid day',
    };
```




---

### 13. Short array syntax

~~Support:~~

```php
~~$arr = [1,2,3];~~
```

~~`lex.c` and `compile.c` must be updated.~~

---

### 4. `function_exists()`


~~Implement `function_exists()`.~~


###5. '' === null


~~empty strings are === null which is wrong.~~



### 18.  Arrow function

~~Implement arrow functions with an explicit `use`:~~

~~$a = fn(): int => .....;          --> ordinary arrow function (lambda), no variables captured from the outer scope!!!~~
~~$a = fn() use($z) : int => .....; --> arrow function with capture (closure), variable $z is imported~~



### 10.  `mixed` type

~~Add mixed type~~

---

### 11.  `enum`


~~Add the `enum` keyword.~~


### 14.  Function return arguments


~~Support for function return types syntax (PHP7.x)~~


### 15.  Overloading:


~~do not let user to register a function with exactly same signature twice. Right now function is overwritten silently.~~


### 16.  Overloading:


~~do not fallback to the last function in the list if there are no good candidates for overloading.
Do fallback only if there is only 1 candidate~~

### 17.  Nullable types:


~~inject code into return statement which LOADC 0,0,0; TEQ ; JNZ over CVT instruction to skip conversion of null to the function type~~

### 18.   `callable` type


~~Implement callable type~~

### 19.  ?? operator 


( ~~?? as a null coalesce OP~~, and ??= null coalesce assignment)

### 20.  ?-> nullsafe operator  


~~Implement a nullsafe arrow operator, which loads NULL. Change the behaviour of -> to generate a VM error if operating on null~~

### 21. Compiler intrinsics: add math functions as opcode


This is required in order to minimize expensive OP_CALL operations on FFI math functions:
abs()       -> OP_ABS
ceil()      -> OP_CEIL
floor()     -> OP_FLOOR
round()     -> OP_ROUND
sqrt()      -> OP_SQRT

sin()       -> OP_SIN
cos()       -> OP_COS
tan()       -> OP_TAN
asin()      -> OP_ASIN
acos()      -> OP_ACOS
atan()      -> OP_ATAN

sinh()      -> OP_SINH
cosh()      -> OP_COSH
tanh()      -> OP_TANH
asinh()     -> OP_ASINH
acosh()     -> OP_ACOSH
atanh()     -> OP_ATANH

exp()       -> OP_EXP
expm1()     -> OP_EXPM1
log()       -> OP_LOG
log10()     -> OP_LOG10



