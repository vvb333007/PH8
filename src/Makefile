TARGET   := libph8.a

CC      := gcc
AR      := ar
CFLAGS  := -MMD -MP -Wall -Wextra -O2 -g  -DDEVEL=1 -DPH7_ENABLE_SQLITE3=0 -DPH7_ENABLE_REGEX=1 -DPH7_ENABLE_THREADS=0 -DADDREFLOG=0 -DPH7_ENABLE_MATH_FUNC=1 -Icygwin -Isqlite3 
CPPFLAGS += -I.

# Subdirectories to build (all directories which have Makefiles)
EXCLUDE  := tmp
SUBDIRS  := $(filter-out $(EXCLUDE),$(patsubst %/Makefile,%,$(wildcard */Makefile)))

# PH8 sources in this directory.
SRCS     := $(wildcard *.c)
OBJS     := $(SRCS:.c=.o)
DEPS     := $(SRCS:.c=.d)

# Objects produced by the subdirectories. This is a shell glob on purpose:
# it is expanded when the archive command runs, i.e. AFTER the sub-makes, so
# freshly built files are seen (make's own $(wildcard) may be evaluated too
# early or from its directory cache).
SUBOBJS  := $(foreach d,$(SUBDIRS),$(d)/*.o)

.PHONY: all clean $(SUBDIRS)

all: $(TARGET)

# ---- subdirectories ------------------------------------------------------

$(SUBDIRS):
	$(MAKE) -C $@

# ---- PH8 itself ----------------------------------------------------------

# Order-only: subdirectories are built first (also with -j), but their
# timestamps do not force PH8 objects to be recompiled.
$(OBJS): | $(SUBDIRS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

# ---- library -------------------------------------------------------------

# $(SUBDIRS) are phony, so the archive is rebuilt on every "make": we cannot
# know from here whether a sub-make changed anything. It is cheap.
# The old archive is removed first so objects deleted from the tree do not
# linger in it.
$(TARGET): $(OBJS) $(SUBDIRS)
	rm -f $@
	$(AR) rcs $@ $(OBJS) $(SUBOBJS)

# ---- clean ---------------------------------------------------------------

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean || exit 1; done
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)