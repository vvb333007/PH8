/* Amalgamated source. Enables 'static' attribute on most of functions
 * Allows for an aggressive compiler optimization, allows better inlining
 */
#define AMALGAM 1

#include "../../src/ph7.h"
#include "../../src/ph7int.h"

#ifdef __CYGWIN__
/* thin layer emulating FreeRTOS calls on CYGWIN */
#include "../../src/cygwin/freertos_shim.c"
#endif

/* HTTP server */
#include "../../src/httpd/httpd.h"
#include "../../src/httpd/httpd.c"

/* Pico HTTP Parser, as is, unmodified */
#include "../../src/httpd/picohttpparser.h"
#include "../../src/httpd/picohttpparser.c"

/* Extensions /Arduino/ESP-IDF specific builtins (emulated in CYGWIN )
 * Code may #define an extension callback 
*/
#include "../../src/esp32/builtins.c"

#ifndef HAS_PH7_RegisterExtension
PH7_PRIVATE sxi32 PH7_VmRegisterExtension(ph7_vm *pVm, const char *zName) {
  pVm = pVm;
  zName = zName;
  return SXRET_OK;
}
#endif

/* PH8 bytecode machine */
#include "../../src/vm.c"

/* FileSystem abstraction */
#include "../../src/vfs.c"

/* POSIX-compatibe File System support */
#include "../../src/posix_vfs.c"

/* Parse. Combines lexems into tokens */
#include "../../src/parse.c"

/* Object model support (classes, methods and so on) */
#include "../../src/oo.c"

/* PHP value object. zval in zend, ph7_value in PH8 */
#include "../../src/memobj.c"

/* Library. Common routines. */
#include "../../src/lib.c"

/* Lexer. Splits file to be compiled to lexems */
#include "../../src/lex.c"

/* PHP arrays */
#include "../../src/hashmap.c"

/* Built-in PHP constants */
#include "../../src/constant.c"

/* Compiler */
#include "../../src/compile.c"

/* Built-in PHP functions */
#include "../../src/builtin.c"

/* Public PH8 API */
#include "../../src/api.c"

#if PH7_ENABLE_REGEX
/* Regexp engine */
#include "../../src/regexp.c"
#endif

#if PH7_ENABLE_SQLITE3
/* SQLite3 engine */
#include "../../src/sqlite3.c"
#endif
