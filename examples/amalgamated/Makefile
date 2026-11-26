# 
# 
.RECIPEPREFIX := >

CC      := gcc
OBJDIR  := .

CFLAGS  := -Wall -Wextra -O2  -MMD -MP -DPH7_ENABLE_SQLITE3=1 -DPH7_ENABLE_THREADS=0 -DPH7_ENABLE_MATH_FUNC=1 -I../../src -I../../src/cygwin -I../../src/httpd -I../../src/sqlite3
COMMON_SRC := amalgam.c

INTER_SRC := $(COMMON_SRC) ph7_interp.c
INTER_OBJ := $(patsubst %.c,$(OBJDIR)/%.o,$(INTER_SRC))

SQLITE3_OBJ := ../../src/sqlite3/sqlite3.o


TARGETS := inter

.PHONY: all

all: $(TARGETS)

$(SQLITE3_OBJ): ../../src/sqlite3/sqlite3.c
>$(MAKE) -C ../../src/sqlite3

inter: $(INTER_OBJ) $(SQLITE3_OBJ)
>$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
>$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
>mkdir -p $(OBJDIR)

clean:
>rm -f *.[od]







