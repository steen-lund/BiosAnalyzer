CC    = gcc
STRIP = strip

TARGET  = BiosAnalyzer
VERSION = 1


OPTIMIZE = -O2
DEBUG    = -g
INCLUDES = -I. -I./include
DEFINES  = 
WARNINGS = -Wall -Wwrite-strings -Werror

CFLAGS  = $(OPTIMIZE) $(DEBUG) $(INCLUDES) $(DEFINES) $(WARNINGS)
LDFLAGS =  
LIBS    = 

STRIPFLAGS = 

SRCS =	Analyzer.c cJSON.c BiosEmulator/besys.c BiosEmulator/biosemu.c BiosEmulator/bios.c BiosEmulator/x86emu/debug.c BiosEmulator/x86emu/decode.c \
    BiosEmulator/x86emu/ops.c BiosEmulator/x86emu/ops2.c BiosEmulator/x86emu/prim_ops.c BiosEmulator/x86emu/sys.c \
	BiosEmulator/pci_accessReg.c

OBJS = $(addprefix obj/,$(SRCS:.c=.o))
DEPS = $(OBJS_000:.o=.d)

.PHONY: all
all: bin/$(TARGET)

-include $(DEPS)

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(ARCH_000) $(CFLAGS) $<
	$(CC) $(CFLAGS) -c -o $@ $<

bin/$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

.PHONY: clean
clean:
	rm -rf bin obj

