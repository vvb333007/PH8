# 
# 
.RECIPEPREFIX := >

# Compiler
#CC      := i686-w64-mingw32-gcc
CC := gcc

# Directories
OBJDIR  := .

# Flags
# -DFIXEDTYPELOG=1
# -DEVEL=1
# -DADDREFLOG=1
# -FRAMELOG=1


CFLAGS  := -Wall -Wextra -O0 -g -MMD -MP  -DDEVEL=1 -DPH7_ENABLE_SQLITE3=1 -DPH7_ENABLE_REGEX=1 -DPH7_ENABLE_THREADS=0 -DADDREFLOG=0 -DPH7_ENABLE_MATH_FUNC=1 -I../../src -I../../src/cygwin -I../../src/sqlite3


# Common sources
COMMON_SRC := ../../src/sqlite3.c ../../src/sqlite3/sqlite3.c ../../src/httpd/httpd.c ../../src/httpd/picohttpparser.c ../../src/cygwin/freertos_shim.c ../../src/api.c ../../src/builtin.c ../../src/compile.c ../../src/constant.c ../../src/hashmap.c ../../src/lex.c ../../src/lib.c ../../src/memobj.c ../../src/oo.c ../../src/parse.c ../../src/vfs.c ../../src/vm.c ../../src/regexp.c ../../src/posix_vfs.c ../../src/esp32/builtins.c 
#COMMON_SRC := ../../src/httpd/httpd.c ../../src/httpd/picohttpparser.c ../../src/cygwin/freertos_shim.c ../../src/api.c ../../src/builtin.c ../../src/compile.c ../../src/constant.c ../../src/hashmap.c ../../src/lex.c ../../src/lib.c ../../src/memobj.c ../../src/oo.c ../../src/parse.c ../../src/vfs.c ../../src/vm.c ../../src/regexp.c ../../src/posix_vfs.c ../../src/esp32/builtins.c ../../src/esp32/constants.c  
#COMMON_SRC := amalgam.c

INTER_SRC := $(COMMON_SRC) ph7_interp.c
INTER_OBJ := $(patsubst %.c,$(OBJDIR)/%.o,$(INTER_SRC))


# Target executable
TARGETS := inter

.PHONY: all

all: $(TARGETS)

inter: $(INTER_OBJ)
>$(CC) $(CFLAGS)  $^ -o $@


$(OBJDIR)/%.o: %.c | $(OBJDIR)
>$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
>mkdir -p $(OBJDIR)

clean:
>rm -f ../../src/*.[od] ../../src/esp32/*.[od] ../../src/cygwin/*.[od]  *.[od]







