.RECIPEPREFIX := >

OBJDIR     := .
AMALG_DIR  := ../amalgamated
MAKE     := make
cc := gcc


CFLAGS  := -Wall -Wextra -O2  -MMD -MP -DPH7_ENABLE_THREADS=0 -DPH7_ENABLE_MATH_FUNC=1 -I../../src -I../../src/cygwin -I../../src/httpd

AMALG_OBJ := $(AMALG_DIR)/amalgam.o

httpd: main.o $(AMALG_OBJ)
>$(CC) $(CFLAGS) $^ ../../src/sqlite3/sqlite3.o -o $@

# FORCE forces 'make' in amalgamated/
$(AMALG_DIR)/amalgam.o: FORCE
>$(MAKE) -C $(AMALG_DIR) amalgam.o

.PHONY: FORCE
FORCE:

clean:
>rm -f *.[od]
