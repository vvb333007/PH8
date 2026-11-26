CC      = gcc
SRCDIR  = ../../src
CFLAGS  = -O2 -Wall -I$(SRCDIR)
LIBPH8  = $(SRCDIR)/libph8.a
LDLIBS  = -lpthread -lm

TARGET  = inter

.PHONY: all clean FORCE

all: $(TARGET)

$(TARGET): ph7_interp.o $(LIBPH8)
	$(CC) -o $@ ph7_interp.o $(LIBPH8) $(LDLIBS)

ph7_interp.o: ph7_interp.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Build (or refresh) the library first; its own Makefile decides what to do.
$(LIBPH8): FORCE
	$(MAKE) -C $(SRCDIR)

FORCE:

clean:
	rm -f *.[od] $(TARGET) $(TARGET).exe

-include ph7_interp.d