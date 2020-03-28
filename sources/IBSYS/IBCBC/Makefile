.SUFFIXES: .map .obj .lst

ASM7090 = asm7090
LNK7090 = lnk7090
OBJ2BIN = obj2bin

ASMFLAGS = -wc 7090 -l $(subst obj,lst,$@)

.map.obj:
	$(ASM7090) $(ASMFLAGS) -o $@ $<

OBJS = c000.obj c100.obj c200.obj c300.obj c400.obj c500.obj c600.obj \
	c700.obj c800.obj c900.obj cz00.obj

all: $(OBJS) rtl
	$(MAKE) -C rtl

clean:
	rm -f *.obj *.lst
	$(MAKE) -C rtl clean

