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

Implement a selected subset of ESP-IDF APIs, starting with the GPIO API.

Implement raw access to MMIO registers.

WiFi bindings, both native and hosted.

---

### 4. Arduino Core bindings and SLAC ("Standard Library of Arduino Classes")

An analogue of the Arduino Core `.cpp` library, implemented in PHP wherever possible.
Standard Arduino Core classes: HardwareSerial, SPI, Wire, FS, Print, String and so on

### 5. Exceptions

~~Rename `Exception` to `Throwable`, and add `Exception`, `Error`, `TypeError`, `ArgumentCountError`, `ArithmeticError`, `DivisionByZeroError`~~

~~Replace `instanceof(Exception)` in `throw` with `Throwable`~~

~~Go throuhg error messages table, and replace them with `VmThrowException` calls~~

Verify failure branches of VmByteCodeExec by provoking the exception

Verify return code propagation of VmByteCodeExec, especially in VmLocalExec use

~~Fix recursion depth limiter~~

### 6. Types and overloading

  Nullable types in stdlib signatures (`?string`, `?Throwable`) or null will not pass strict checks: null will never match ?type signature when overloading

  "No matching function" message should print argument types, and the list of candidates

  Constructor by class name - find places where it is'nt handled (`new`, `parent::`, `method_exists`)

## 7. VM shutdown

  ~~Call `__destruct` for all live objects on VM exit (globals are not destructed right now on script DONE), before function and class tables are freed~~

~~Decide what `OP_HALT` does with destructors, and write it down~~

~~Objects with a cyclic references never reach zero refcount, need a list of all live objects~~

---

## Smaller tasks


### Magic methods

__invoke() does not receive any arguements right now. __call and __callStatic do not receive method name

implement __terminate() magic method
implement __oom() magic method
implement __toArray() magic method


### 1. '?:' Elvis operator support

Implement `$a = $b ?: $c;`

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

Currently do not receive called method name.


### 6. UNIX-like `fork()` to make a full clone of a VM

Start a new VM by cloning the current VM.

```php
$pid = fork();

if ($pid == 0) {
    echo 'Child running!';
} else {
    echo 'Child has been spawned, pid=' . $pid;
}
```
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

VM1:

```php
php_ipc_announce('My Fancy Name');
```

VM2:

```php
$handle = php_ipc_bind(
    $announced_name,
    $auth_creds_or_null,
    $timeout
); // returns the real FreeRTOS task/VM handle as a ph7_value

$err = php_ipc_send($handle, $value); // send an arbitrary PHP value
$value = php_ipc_recv($handle, $timeout);

php_ipc_wake($handle, $value); // send a 32-bit scalar to wake up a sleeping VM
php_ipc_sleep($handle, $mask); // sleep until woken up by another VM
                                  // ($value & $mask != 0 => wake up)
```

---

### 9. C++-style constructors

Not always checked for existence. Should we patch a constructor lookup code?



### 12. `match` keyword

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



