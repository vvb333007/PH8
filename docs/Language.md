# PH8 vs Zend PHP: Language and Implementation Differences

This text is an AI translation of original file in Russian.

## 1. Features Not Yet Supported by PH8

- **Short destructuring syntax:** `[$a, $b, $c] = $x;` is not supported yet; it is planned.
- **`list` as the target after `as` in `foreach`:** not supported.
- **`[]` as the target after `as` in `foreach`:** not supported yet; it is planned.
- **The `??=` operator:** not supported.
- **Constructor property promotion:** not supported. Class properties must be declared explicitly.
- **`__set()` and some other magic methods:** not supported. Similar functionality can be implemented in PH8 through `__get()`.

## 2. PH8 Features That Differ from PHP

The biggest incompatibility between PH8 and Zend PHP 8 is that arrays are always passed and returned by reference. In other words, the semantics are closer to C: passing an array passes a pointer to it. If a function needs its own copy of an array, it must create one explicitly: `$local = array_copy($arg);`

```php
$a = array(1,2,3,4);

function hello($az) {

  $az[0] = 10;
  var_dump($az);
}

hello($z);
var_dump($a); // The first element will be 10
```

For the same reason, assigning an array assigns a reference to it, not a copy of the array:

```php
$arr = array(1,2,3);
$b = $arr;
$b[0] = 10;
var_dump($arr); // The first element is 10
```

This design reflects the memory constraints of embedded environments. If arrays were copied every time they were passed around, memory would quickly be exhausted by keeping multiple copies around, allocating them, and then immediately freeing them.

This behavior will be familiar to C and C++ programmers.

### 2.1. Integer Arithmetic and UTF-8

PH8 uses 64-bit integer arithmetic on all platforms. In PHP, the size of an integer depends on the platform: 32 bits on 32-bit platforms and 64 bits on 64-bit platforms.

PH8 uses `double` rather than `float` on all platforms.

PH8 also has native UTF-8 support for strings, identifiers, constants, variables, function names, class names, and similar entities.

### 2.2. Accessing Static Properties Through an Object

In PH8, static properties can be accessed and assigned through an object using the `->` operator. Zend PHP does not allow this syntax for static properties.

```php
class MyClass {
    public static $a = 5 * 5;
}

$n = new MyClass;
$n->a <<= 1;

echo MyClass::$a; // Prints 50
```

### 2.3. Destructor Calls on Failure

If the program crashes, the destructor of each class instance is called. If defined, the special magic method `__terminate(bool $is_oom, ?string $reason)` is called as well. It is intended for emergency cleanup of hardware resources allocated by the class.

On larger systems, the operating system usually handles cleanup. In embedded systems, that responsibility often falls to the application developer.

### 2.4. Constructor and Destructor Overloading

In PH8, constructors and destructors can be overloaded just like other class methods.

If a class has no constructor, its properties retain their initialized values, including values produced by complex expressions, rather than being reset to zero or null.

Zend PHP does not support this kind of overloading, and its support for complex expressions in initializers is more limited.

### 2.5. Complex Expressions in Default Values and Constants

In PH8, function arguments can have default values defined by complex expressions. The same approach applies to `const`, `static`, and `enum` values.

For example:

```php
const Hello = 'Hello' . rand_str(15);

function test($callback = function() {
    echo "Hello World!\n";
}) {
    // Check that we have a callback
    if (!is_callable($callback)) {
        die("Expecting a callback");
    }

    // Invoke the callback
    $callback();
}

// Call without arguments
test(); // Prints: Hello World!

// Call with one argument
test(function() {
    echo "Welcome guest\n";
}); // Prints: Welcome guest

test(10); // Prints: Expecting a callback
```

### 2.6. Function Overloading by Argument Count and Type

PH8 supports function overloading by argument count and type. Zend PHP does not support this mechanism.

```php
function foo(int $a) {
    echo "a is integer\n";
    var_dump($a);
}

function foo(array $a) {
    echo "a is an array\n";
    var_dump($a);
}

foo(52); // a is integer
foo(array(14, __TIME__, __DATE__)); // a is an array
```

### 2.7. The Comma Operator

In PH8, the comma operator can be used in expressions.

Multiple `case` entries in an `enum` can also be separated by commas, provided each one has an explicit value.

Zend PHP has no comma operator as such. Commas are part of language constructs and can only be used within them, for example in `$a = [1,2,3]`, but not as an operator in an expression.

```php
$a = 25, $b = $a << 1, test();

// Print the values of $a and $b
function test() {
    global $a, $b;
    echo "\$a = $a \$b= $b\n"; // $a = 25 $b = 50
}

enum Hello: int {
    case A = 1, B = 2, C = 3;
    case H = 66;
}
```

## 3. Type System and Automatic Conversion

Unlike Zend PHP, PH8 treats a declared type as a guarantee, not a restriction. The virtual machine automatically converts a value when necessary to maintain that guarantee.

### Type Guarantees in PH8

- **Input Guarantee**

  Code inside a function or method can rely on the declared types of its arguments, regardless of the values actually passed by the caller.

- **Output Guarantee**

  The code calling a function or method can rely on its declared return type, regardless of the value actually returned by the function.

- **Encapsulated Data-Type Guarantee**

  Methods and other class members can rely on the declared types of an object's properties. A typed property always contains a value of its declared type, regardless of how the value was assigned.

### Details

As in PHP 8, PH8 lets you declare argument types for functions and methods, as well as return types.

The difference is how these declarations work.

In PHP 8, a type mismatch is treated as an **error**. In PH8, a declared type is instead an **instruction to the virtual machine**: it tells the VM **which type to convert the value to**.

The same rules apply to arguments and return values. The type is therefore guaranteed both when entering and when returning from a function.

### Scalar Types

Consider this function declaration:

```php
function test(int $a, ?string $b) { /* ... */ }
```

This call is valid:

```php
test('66', 12);
```

The VM converts the string `'66'` to the integer `66` because the first argument is declared as `int`. The value `12` is converted to the string `'12'` because the second argument is declared as `?string`.

The same applies to return values. If a function declared with return type `: int` returns `'66'`, the caller receives `66`.

### Untyped and Nullable Values

If no type is declared, no automatic conversion takes place. The type is treated as `mixed`, and the value is passed through unchanged.

For a nullable type such as `?string`, conversion applies only to values other than `null`. A `null` value is passed through unchanged.

### The `object` Type

If the value passed or returned is already an object, it is passed through unchanged.

If the value is not an object, it is automatically wrapped in `stdClass($scalar_value)`. The original value is preserved, and code expecting an object receives one.

The same rule applies to `null`. If an argument or return value is declared as `object` without `?`, `null` is also converted to `stdClass(null)`.

Use `?object` if `null` should pass through unchanged.

### The `callable` Type

A value is considered a valid `callable` if it is a string or array identifying an existing function or method that can be called. For example:

```php
'strlen'
```

or:

```php
[$obj, 'method']
```

If a value cannot be converted to a `callable`, it is replaced with a special function named `__badcallable` from the system library.

The resulting value behaves like a valid `callable`: `is_callable()` returns `true`. However, the first attempt to invoke it throws a `TypeError`, which can be caught with `try/catch`.

This way, the error is not lost. It occurs when the invalid value is actually used, rather than going unnoticed.

PH8 always guarantees the declared type: scalar values are converted, non-objects (including `null`) are wrapped in `stdClass`, and an invalid `callable` is replaced with a stub that throws `TypeError` when invoked.

### Typed Class Properties

Class properties can also have declared types:

```php
class Test {
  public static int $z = 0;
  public ?string $a = 'hello';
}
```

A property's declared type is **fixed**; assigning a value of a different type cannot change it.

When a value is assigned to a typed property, PH8 converts it to the declared type if possible. The property therefore retains its type for the lifetime of the object.

For example:

```php
$a = new Test();

$a->a = 123;
```

The value is automatically converted to the string `'123'` because `$a->a` is declared as `?string`.

Class properties follow the same conversion rules as function arguments and return values.

## 4. Arrow Functions

PH8 arrow functions require outer variables to be captured explicitly with `use`. The return value is converted to the declared type.

```php
fn($x) use ($factor): int => $x * $factor
```

In PH8, `$factor` must be captured explicitly. Zend PHP arrow functions automatically capture variables from the outer scope.

## 5. Enumerations (`enum`)

PH8's `enum` implementation differs from Zend PHP in several ways.

### 5.1. Pure Enums

In PH8, cases in a pure `enum` are automatically generated constants. In PHP, pure enum cases do not have associated scalar values.

### 5.2. Backed Enums and Value Conversion

In PH8, enum cases can have values of different types, including the results of complex expressions and function calls, even when the enum declares a backing type. The declared type is used to convert the value automatically when it is consumed in a context that expects that type.

In Zend PHP, every backed enum case must match the declared scalar backing type, such as `int` or `string`.

Values of an enum declared with type `string` are converted to strings when used as that type.

### 5.3. Mixed Types and Automatic Numbering

PH8 allows enum cases to have values of different types without checking their types at declaration time. Zend PHP has no direct equivalent for this kind of backed enum.

PH8 also supports automatic generation of case values. Automatic numbering can be mixed with explicit values; implicit values continue to be generated independently.

### 5.4. Enum Examples

```php
// Enum without automatic conversion (mixed)
enum Loo: mixed {
    case Name1 = 10;
    case Name2 = 10.2;
    case Name3 = 'Hello' . rand_str(4);
}

// When used as Suit $x in a function argument,
// the value is automatically converted to a string
enum Suit: string {
    case Name1 = 10;
    case Name2 = 10.2;
    case Name3 = 'Hello' . rand_str(4);
}

function test(Suit $x) { // Equivalent to test(string $x)
    var_dump($x);         // Prints the value as a string
}

function test(Loo $x) {  // Equivalent to test(mixed $x)
    var_dump($x);        // Prints the value with its original type
}
```

## 6. Closures (`Closure`), `never`, and `callable`

### 6.1. Closure Representation

In PH8, a closure is represented as `string|array`. For example:

```php
$a = function() use($x,$y) {};
var_dump($a);
```

This prints something like `string(9, 'closure_1')`.

In Zend PHP, a closure is an object with properties and methods.

### 6.2. The `never` Return Type

In PH8, declaring `:never` automatically inserts a `HALT` opcode if execution reaches the function's return point. No exception is thrown; the program is halted instead.

In Zend PHP, compliance with `never` is checked at compile time, although static analysis cannot always verify every case.

## 7. Operators

### 7.1. The `?:` (Elvis) and `??=` Operators Are Not Supported

Use the regular ternary operator `? a : b` and the null coalescing operator `??` instead.

### 7.2. The `?->` (Nullsafe) Operator

In PH8, the nullsafe operator applies only to the method call or property access immediately following it; null propagation does not continue automatically.

In Zend PHP 8, `$a?->method()->method()` only needs one nullsafe operator. If the first nullsafe operator encounters `null`, the whole expression evaluates to `null`.

The equivalent PH8 expression is `$a?->method()?->method()`: each `->` in the chain must have its own nullsafe operator.

## 8. Language Constructs

In PH8, `echo` is a function and can be used in an expression. `is_callable('echo')` returns `true`.

`yield` (generators) and `match` are not supported.

Namespaces are not supported. The parser recognizes the syntax but does not implement its semantics.

## 9. Exceptions and Exception Handling

In Zend PHP, `Error` and `Exception` implement the `Throwable` interface. To create a class that can be used with `throw`, it must implement `Throwable`:

```php
class MyClass implements Throwable {
...
}
```

In PH8, `Throwable` is a regular class rather than an interface. Both `Error` and `Exception` inherit from it:

```php
class MyClass extends Throwable {  // extends instead of implements
...
}
```

This means that `throw new Throwable()` is valid and will compile and run.

The VM can throw exceptions while executing a script, for example on division by zero, calls to unknown functions, or access to unknown object properties.

However, if an exception occurs while initializing a class's static properties or constants, the compiler marks the entire class as invalid. Attempting to instantiate the class returns `NULL`; attempting to access its properties throws an exception.

For example:

```php
class MyClass {
  const A = 1 / 0;  // <-- division by zero
}
```

This class will compile and the script will start, but an instance of the class cannot be created.

If an exception occurs while initializing a regular property (neither a constant nor a static property), the program does not stop, but the error is not silently ignored either: an error is reported and execution continues. The property whose initialization threw the exception is set to `null`.

An exception is also thrown if an error occurs in a function or method's default argument expression. For example:

```php
function test(int $a = 1/0) {
}

test();
```

**These exceptions cannot be caught in the usual way.** After the `catch` block runs, execution resumes at the instruction following the one that triggered the exception:

```php
try {
  test();                        // 1. Call test() without arguments; the default value is used.
  die ("Oops");                  // 3. Execution continues here.
} catch(Error $e) {
  echo 'Caught!', PHP_EOL;       // 2. Catch the exception.
}
```
