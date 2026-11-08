<?php
/*
 * Namespace test for PH8.
 * Every global function, constant and class used inside a namespace is written with
 * the leading backslash (no fallback to the global namespace yet).
 * The script uses only bracketed namespaces, so it is valid PHP too.
 */

namespace {
    class T {
        public static $pass = 0;
        public static $fail = 0;

        public static function check($sLabel, $mGot, $mWant) {
            if ($mGot === $mWant) {
                self::$pass++;
                echo "ok    ", $sLabel, "\n";
            } else {
                self::$fail++;
                echo "FAIL  ", $sLabel, ": got '", $mGot, "', want '", $mWant, "'\n";
                var_dump($mGot);
                var_dump($mWant);
            }
        }

        public static function summary() {
            echo "\n", self::$pass, " passed, ", self::$fail, " failed\n";

        }
    }

    function global_func() {
        return "global";
    }

    T::check("__NAMESPACE__ in the global namespace", __NAMESPACE__, '');
}

namespace App\Util {
    const VERSION = "1.0";

    function greet($sName) {
        return "Hello, " . $sName;
    }

    function who() {
        return "App\\Util";
    }

    interface Shape {
        public function area();
    }

    class Square implements Shape {
        public $iSide = 3;
        public function area() {
            return $this->iSide * $this->iSide;
        }
    }

    class Str {
        const PREFIX = ">>";

        public static function mark($sText) {
            return self::PREFIX . $sText;
        }

        public function className() {
            return __CLASS__;
        }
    }

    class UtilException extends \Exception {
    }

    \T::check("__NAMESPACE__", __NAMESPACE__, "App\\Util");

    // Unqualified names resolve inside the current namespace
    \T::check("unqualified function call", greet("PH8"), "Hello, PH8");
    \T::check("unqualified constant", VERSION, "1.0");
    \T::check("unqualified class: static call", Str::mark("a"), ">>a");
    \T::check("unqualified class: constant", Str::PREFIX, ">>");
    $oStr = new Str();
    \T::check("new + __CLASS__ is a full name", $oStr->className(), "App\\Util\\Str");
    \T::check("get_class is a full name", \get_class($oStr), "App\\Util\\Str");

    // Fully qualified and relative names
    \T::check("fully qualified function", \App\Util\greet("x"), "Hello, x");
    \T::check("fully qualified constant", \App\Util\VERSION, "1.0");
    \T::check("relative function (namespace\\name)", namespace\greet("y"), "Hello, y");
    \T::check("global function with leading backslash", \global_func(), "global");
    \T::check("global builtin with leading backslash", \strlen("abcd"), 4);

    // Interfaces and instanceof
    $oSquare = new Square();
    \T::check("class implements interface", $oSquare instanceof Shape, true);
    \T::check("instanceof fully qualified", $oSquare instanceof \App\Util\Shape, true);
    \T::check("method of a namespaced class", $oSquare->area(), 9);

    // Exceptions
    try {
        throw new UtilException("boom");
    } catch (UtilException $e) {
        \T::check("catch with an unqualified name", $e->getMessage(), "boom");
    }
    try {
        throw new UtilException("boom2");
    } catch (\Exception $e) {
        \T::check("catch the global parent class", $e->getMessage(), "boom2");
    }
}

namespace App\Other {
    // The same names as in App\Util, in another namespace: no collision
    function who() {
        return "App\\Other";
    }

    class Str {
        public static function mark($sText) {
            return "##" . $sText;
        }
    }

    \T::check("same function name, other namespace", who(), "App\\Other");
    \T::check("same class name, other namespace", Str::mark("a"), "##a");
    \T::check("other namespace does not see App\\Util", \App\Util\who(), "App\\Util");
}

namespace App\Main {
    use App\Util\Str;
    use App\Util\Str as S;
    use App\Util\Shape;
    use App\Other;
    use function App\Util\greet;
    use function App\Util\who as util_who;
    use const App\Util\VERSION;
    use App\Util\{Square, UtilException as UE};

    function who() {
        return "App\\Main";
    }




    \T::check("__NAMESPACE__ in App\\Main", __NAMESPACE__, "App\\Main");
    \T::check("use class", Str::mark("a"), ">>a");
    \T::check("use class as alias", S::mark("b"), ">>b");
    \T::check("use namespace, qualified name", Other\Str::mark("c"), "##c");
    \T::check("use function", greet("PH8"), "Hello, PH8");
    \T::check("use function as alias", util_who(), "App\\Util");
    \T::check("own function is not the imported one", who(), "App\\Main");
    \T::check("use const", VERSION, "1.0");
    $oGroup = new Square();
    \T::check("group use", $oGroup->area(), 9);
    \T::check("class alias is case-insensitive", s::mark("d"), ">>d");

    try {
        throw new UE("grouped");
    } catch (UE $e) {
        \T::check("group use with alias in new/catch", \get_class($e), "App\\Util\\UtilException");
    }

    // Type hints with imported and qualified names
    function area_of(Shape $oShape) {
        return $oShape->area();
    }
    function make_square(): Square {
        return new Square();
    }
    function make_other(): \App\Other\Str {
        return new \App\Other\Str();
    }
    \T::check("parameter type hint (imported name)", area_of(new Square()), 9);
    $oMade = make_square();
    \T::check("return type (imported name)", $oMade->area(), 9);
    \T::check("return type (fully qualified)", \get_class(make_other()), "App\\Other\\Str");

    // Names built at run time are always complete names
    $sClass = 'App\Util\Str';
    $oDyn = new $sClass();
    \T::check("new from a string", $oDyn->className(), "App\\Util\\Str");
    \T::check("class_exists with a string", \class_exists('App\Util\Str'), true);
    \T::check("function_exists with a string", \function_exists('App\Util\greet'), true);
    \T::check("call_user_func with a string", \call_user_func('App\Util\greet', "dyn"), "Hello, dyn");
    \T::check("constant() with a string", \constant('App\Util\VERSION'), "1.0");

    // Names are case-insensitive, constants are not
//    \T::check("function name case", \app\util\GREET("c"), "Hello, c");
    \T::check("class name case", \app\util\str::mark("e"), ">>e");
}

// A new namespace block starts without the imports of the previous one
namespace App\Isolated {
    function probe() {
        return __NAMESPACE__;
    }
    \T::check("imports are forgotten in the next block", probe(), "App\\Isolated");
}

namespace {
    T::check("back in the global namespace", __NAMESPACE__, "");
    T::check("global code sees global functions", global_func(), "global");
    T::summary();
}




?>
