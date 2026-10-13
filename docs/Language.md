# PH8 vs Zend PHP: Language and Implementation Differences

## 1. Features Not Yet Available in PH8

- **Namespaces** are not supported (the parser can parse them, but they are not used); not planned.
- **Destructuring with the short `list` syntax:** `[$a, $b, $c] = $x;` is not supported yet; it is planned.
- **Using `list` after `as` in `foreach`:** not supported.
- **Using `[ ]` after `as` in `foreach`:** not supported yet; planned.
- **The `??=` operator:** not available.
- **Constructor property promotion:** not supported. Class properties must be declared explicitly.
- **`__set()` and some other magic methods:** not supported. The functionality of `__set()` can be achieved in PH8 in another way, through `__get()`.

## 2. PH8 Features That Differ from PHP



The biggest incompatibility between PH8 and Zend PHP 8 is that arrays are always passed (and returned) by reference. In other words, the semantics are more C-like, where passing an array means passing a pointer to it. If a function needs its own copy of an array to work with, it must create this copy itself: `$local = array_copy($arg);`

```php
$a = array(1,2,3,4);

function hello($az) {

  $az[0] = 10;
  var_dump($az);
}

hello($z);
var_dump($a); // The first element will be 10
```

Accordingly, array assignment operatons assign a reference, not the array itself:

```php
$arr = array(1,2,3);
$b = $arr;
$b[0] = 10;
var_dump($arr); // The first element of the array is 10
```

This behaviour is related to memory limitations in embedded environments. If a careless programmer passes arrays by copy everywhere, there will simply not be enough resources to keep all these copies in memory, create them and then immediately destroy them.

For C/C++ programmers, this behaviour is quite familiar.

### 2.1. Integer Arithmetic and UTF-8

PH8 uses 64-bit integer arithmetic on all platforms. In PHP, the integer size depends on the platform: 32 bits on 32-bit platforms and 64 bits on 64-bit ones.

PH8 uses `double` instead of `float` on all platforms.

PH8 also has native UTF-8 support for strings, identifiers, constants, variables, function names, class names and other similar entities.

### 2.2. Accessing Static Properties Through an Object

In PH8, static properties can be accesed and assigned values through an object using the `->` operator. In PHP, this syntax is not allowed for static properties.

```php
class MyClass {
    public static $a = 5 * 5;
}

$n = new MyClass;
$n->a <<= 1;

echo MyClass::$a; // Prints 50
```

### 2.3. Calling Destructors on a Crash

In case of a crash, the destructor of each class instance will be called, as well as a special magic method `__terminate(bool $is_oom, ?string $reason)`, if it is defined. This method is intended for emergency release of hardware resources, if any were allocated by the class.

On large systems, the operating system handles cleanup. In embedded systems, this is often the programmer's responsibility instead.

### 2.4. Constructor and Destructor Overloading

In PH8, constructors and destructors can be overloaded in the same way as other class methods.

If constructor is not present, properties keep their initialization values, including values obtained from complex expressions, instead of being reset to zero.

Zend PHP does not support this kind of overloading, and the use of complex expressions is more limited.

### 2.4. Complex Expressions in Default Values and Constants

In PH8, function arguments can have complex expressions as default values. The same approach applies to `const`, `static` and `enum` values.

Example:

```php
const Hello = 'Hello' . rand_str(15);

function test($callback = function() {
    echo "Hello World!\n";
}) {
    // Check that we have a callback
    if (!is_callable($callback)) {
        die("Expecting a callback");
    }

    // Call the callback
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

### 2.5. Function Overloading by Number and Types of Arguments

PH8 supports function overloading by the number and types of arguments. Zend PHP does not support this mechanism.

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

### 2.6. The Comma Operator

In PH8, the comma operator can be used in expressions.

It is also possible to list several `case` entries in an `enum`, separated by commas, if explicit values are assigned to them.

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

Unlike Zend PHP, in PH8 a declared type is a guarantee, not a restriction. The virtual machine automatically converts a value to the required type when it is necessary to keep this guarantee.

### Type Guarantees in PH8

- **Input Guarantee**

  Code inside a function or method can rely on the declared types of its arguments, regardless of what the calling code actually passed.

- **Output Guarantee**

  Code calling a function or method can rely on the declared return type, regardless of what the function actually returned.

- **Encapsulated Data-Type Guarantee**

  Methods and other class members can rely on the declared types of object properties. A typed property always contains a value of its declared type, regardless of how the value was assigned.

### More Details

As in PHP 8, PH8 allows you to declare types for function and method arguments, as well as return values.

But the principle behind it is different.

In PHP 8, a type mismatch is considered an **error**. In PH8, the declared type is an **instruction to the virtual machine**: it tells the VM **which type the value should be converted to**.

The same rules apply to arguments and return values. Therefore, the type is guaranteed both when entering a function and when leaving it.

### Scalar Types

Suppose a function is declared like this:

```php
function test(int $a, ?string $b) { /* ... */ }
```

Then this call is perfectly valid:

```php
test('66', 12);
```

The virtual machine converts the string `'66'` to the number `66`, because the first argument has type `int`. The number `12` is converted to the string `'12'`, because the second argument has type `?string`.

The same thing happens with return values. If a function with return type `: int` returns `'66'`, the calling code receives `66`.

### Missing Types and Nullable Types

If no type is specified, no automatic conversion takes place. In this case, the type is considered to be `mixed`, and the value is passed as it is.

If a type is declared as nullable, for example `?string`, conversion is applied only to values other than `null`. The `null` value itself is passed without changes.

### The `object` Type

If an object value is passed or returned, it is passed as it is.

If the value is not an object, it is automatically wrapped in `stdClass($scalar_value)`. The original value is not lost, and code expecting an object receives an object in any case.

The same applies to `null`. If an argument or return value has type `object` without `?`, then `null` is also converted to `stdClass(null)`.

If `null` should pass without changes, use the `?object` type.

### The `callable` Type

A value is considered a valid `callable` if it is a string or an array pointing to an existing function or method which can actually be called. For example:

```php
'strlen'
```

or:

```php
[$obj, 'method']
```

If a value cannot be converted to a `callable`, it is replaced with a special function called `__badcallable` from the system library.

For the rest of the code, it is a normal `callable`: `is_callable()` returns `true`. But at the first attempt to call it, a `TypeError` exception will be thrown, which can be caught using `try/catch`.

This way the error is not lost. It happens when the invalid value is actually used, instead of remaining unnoticed.

PH8 always guarantees that the declared type is respected: scalars are converted, non-objects (including `null`) are wrapped in `stdClass`, and an invalid `callable` is replaced with a stub which throws `TypeError` when called.

### Typed Class Properties

Class properties can also have a declared type:

```php
class Test {
  public static int $z = 0;
  public ?string $a = 'hello';
}
```

The declared property type is **permanent**. It cannot be changed by assigning a value of another type.

When a value is written to a typed property, PH8 automatically converts it to the declared type, if possible. This way, the property keeps its type during the whole lifetime of the object.

For example:

```php
$a = new Test();

$a->a = 123;
```

The value will be automatically converted to the string `'123'`, because `$a->a` is declared as `?string`.

Class properties follow the same conversion rules as function arguments and return values.

## 4. Arrow Functions

PH8 arrow functions require explicit capture of external variables using `use`. The return value is converted to the declared type.

```php
fn($x) use ($factor): int => $x * $factor
```

In PH8, the external variable `$factor` is captured explicitly. In Zend PHP, arrow functions automatically capture variables from the outer scope.

## 5. Enumerations (`enum`)

The `enum` mechanism in PH8 differs from the Zend PHP implementation in several ways.

### 5.1. Ordinary Enumerations

In PH8, cases of an ordinary `enum` are described in the original notes as automatically generated constants. In PHP, cases of a pure enumeration do not have associated scalar values.

### 5.2. Backed Enums and Value Conversion

In PH8, enum cases can have values of different types, including results of complex expressions and function calls, even if the enum itself has a declared backing type. The declared type is used to automatically convert the value when it is consumed in a context requiring that type.

In Zend PHP, every backed enum case must match the declared scalar type, for example `int` or `string`.

The original notes also specify that values of a `string` enum are converted to strings when used as that type.

### 5.3. Mixed Types and Automatic Numbering

PH8 allows enums with case values of different types without checking their types at declaration time. There is no general equivalent of this kind of backed enum in Zend PHP.

PH8 also supports automatic generation of case values. According to the original notes, automatic numbering can be combined with explicit values: implicit values continue to be generated independently.

### 5.4. Enum Examples

```php
// Enum without automatic conversion (mixed)
enum Loo: mixed {
    case Name1 = 10;
    case Name2 = 10.2;
    case Name3 = 'Hello' . rand_str(4);
}

// When used as Suit $x in function arguments,
// the value is automatically converted to a string
enum Suit: string {
    case Name1 = 10;
    case Name2 = 10.2;
    case Name3 = 'Hello' . rand_str(4);
}

function test(Suit $x) { // Equivalent to test(string $x)
    var_dump($x);         // Prints a string value
}

function test(Loo $x) {  // Equivalent to test(mixed $x)
    var_dump($x);        // Prints the value with its actual type
}
```

## 5. Closures (`Closure`), `never` and `callable`

### 5.1. Closure Representation

In PH8, a closure is represented as a `string|array`. For example:

```php
$a = function() {};
var_dump($a);
```

will print something like `string(9, 'closure_1')`.

In Zend PHP, a closure is an object with its own properties and methods.

### 5.2. The `never` Return Type

In PH8, the `:never` modifier automatically inserts the `HALT` opcode if execution reaches the point where the function would return.

In Zend PHP, compliance with `never` is checked at compile time, although, according to the original notes, static analysis is not always able to verify this in all cases.

## 6. Operators

### 6.1. The `?:` (Elvis) and `??=` Operators Are Not Supported

Use the ordinary ternary operator `? a : b` and the regular null coalescing operator `??` instead.

### 6.2. The `?->` (Nullsafe) Operator Only Applies to Its Own Method Call

In PH8, null propagation does not happen automatically across the rest of the expression.

Zend PHP 8:

```php
$a?->method()->method()
```

The nullsafe operator is needed only once. The whole expression evaluates to `null` if the first nullsafe operator encounters `null`.

The equivalent expression in PH8 is:

```php
$a?->method()?->method()
```

The nullsafe operator must be specified at every arrow.

## 7. Language Constructs

`echo` is a function and can be used inside an expression. `is_callable('echo') == TRUE`.

`yield` (generators) and `match` are not supported.

